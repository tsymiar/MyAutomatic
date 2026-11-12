import Foundation
import FileTransferCore

// ──────────────────────────────────────────────────────────────────────────────
//  TransferCore — the send flow (serial queue + single-file send)
//  Sending blocks, so it runs on a background thread and comes back to the
//  main thread to update state.
// ──────────────────────────────────────────────────────────────────────────────

@MainActor
extension TransferCore {

    // MARK: - Send file (client mode, blocking → run on background)

    func postLocalFile(_ filePath: String) {
        sendQueue.append(filePath)
        drainSendQueue()
    }

    private func drainSendQueue() {
        guard !isBusy, !sendQueue.isEmpty else { return }
        // A new transfer invalidates any pending auto-disconnect timer.
        disconnectToken &+= 1
        performSend(sendQueue.removeFirst())
    }

    private func performSend(_ filePath: String) {
        guard isConnected, let engine = handle else {
            transferStatus = "Not connected"
            return
        }

        let taskID = startSendTask(filePath)
        let task = transferTasks.last(where: { $0.id == taskID })
        let fileName = task?.fileName ?? ""
        let fileSize = task?.fileSize ?? 0

        Task.detached { [weak self, engine, filePath, taskID, fileName, fileSize] in
            let ret = ft_send_file(engine, filePath)
            await MainActor.run { [weak self] in
                self?.finishSend(taskID: taskID, fileName: fileName, fileSize: fileSize, ret: ret)
            }
        }
    }

    /// Create the task row and prime the status bar for a new send; returns the task id.
    private func startSendTask(_ filePath: String) -> UUID {
        let url = URL(fileURLWithPath: filePath)
        let fileName = url.lastPathComponent
        let fileSize = (try? url.resourceValues(forKeys: [.fileSizeKey]).fileSize).map(UInt64.init) ?? 0

        isBusy = true
        inflightCalls += 1
        transferStatus = "Preparing..."
        transferFileName = fileName
        transferredBytes = 0; totalBytes = fileSize; progress = 0
        appendLog("Sending: \(fileName) (\(formatBytes(fileSize)))")

        var task = TransferTask(
            fileName: fileName, fileSize: fileSize,
            direction: .sending, peerIP: targetIP
        )
        task.status = .transferring
        transferTasks.append(task)
        inFlightTaskIDs.append(task.id)
        return task.id
    }

    /// Handle the result of ft_send_file on the main thread.
    private func finishSend(taskID: UUID, fileName: String, fileSize: UInt64, ret: Int32) {
        endEngineCall()
        // isBusy must be cleared even if History was cleared mid-transfer.
        isBusy = false
        inFlightTaskIDs.removeAll { $0 == taskID }
        let idx = transferTasks.firstIndex { $0.id == taskID }
        switch ret {
        case 0:
            transferStatus = "Send complete"; progress = 1.0
            appendLog("Send complete: \(fileName)")
            if let idx { transferTasks[idx].status = .completed }
        case -2:
            // CANCELLED callback already set .cancelled - do not overwrite with .failed.
            transferStatus = "Send cancelled"
            appendLog("Send cancelled: \(fileName)")
            pendingPartialNames.append(fileName)
            // Cancelling stops the whole queue; remaining files are not sent.
            sendQueue.removeAll()
        default:
            transferStatus = "Send failed (code \(ret))"
            appendLog("Send failed: \(fileName) (code \(ret))")
            if let idx { transferTasks[idx].status = .failed }
        }
        if let idx {
            transferTasks[idx].bytesTransferred =
                ret == 0 ? fileSize : transferTasks[idx].bytesTransferred
        }
        continueSendQueue(after: ret)
    }

    /// Continue draining the queue, or auto-disconnect after a clean success.
    private func continueSendQueue(after ret: Int32) {
        if !sendQueue.isEmpty {
            drainSendQueue()
        } else if ret == 0 && isConnected && pendingPartialNames.isEmpty {
            // Auto-disconnect only after success: a cancel keeps the connection for CMD_DELETE.
            scheduleAutoDisconnect(after: 1.0)
        }
    }

    // MARK: - Pull file from peer (client mode, blocking → run on background)

    /// Pull a file from the peer.  ft_request_file blocks until the transfer
    /// finishes; isBusy stays true meanwhile to disable the disconnect button
    /// (destroying the engine while in use is use-after-free).  The task row
    /// is created by the FT_EVT_RECEIVE_START callback with the real size.
    func requestFile(_ fileName: String) {
        let name = fileName.trimmingCharacters(in: .whitespacesAndNewlines)
        guard !name.isEmpty else { return }
        guard !isBusy else { return }
        guard isConnected, let engine = handle else {
            transferStatus = "Not connected"
            return
        }
        guard let port = UInt16(targetPort), port > 0 else {
            transferStatus = "Invalid port number: \(targetPort)"
            appendLog("ERROR: Invalid port: \(targetPort)")
            return
        }
        let host = targetIP
        disconnectToken &+= 1

        isBusy = true
        inflightCalls += 1
        transferStatus = "Requesting \(name)…"
        transferredBytes = 0; totalBytes = 0; progress = 0
        appendLog("Pulling from \(host): \(name)")

        Task.detached { [weak self, engine, host, port, name] in
            let ret = ft_request_file(engine, host, port, name)
            await MainActor.run { [weak self] in
                guard let self else { return }
                self.endEngineCall()
                self.isBusy = false
                if ret == 0 {
                    self.transferStatus = "Pull complete"
                    self.appendLog("Pull complete: \(name)")
                    // Status + on-disk path handled by the FT_EVT_TRANSFER_COMPLETE callback
                } else if ret == -8 {
                    // Cancelled (local/peer): the CANCELLED callback already set .cancelled.
                    self.transferStatus = "Pull cancelled"
                    self.appendLog("Pull cancelled: \(name)")
                } else {
                    self.transferStatus = ret == -5
                        ? "File not found on peer: \(name)"
                        : "Pull failed (code \(ret))"
                    self.appendLog("ERROR: Pull failed: \(name) (code \(ret))")
                    if let idx = self.transferTasks.lastIndex(where: {
                        $0.fileName == name && $0.direction == .receiving
                            && $0.status != .completed
                    }) {
                        self.transferTasks[idx].status = .failed
                    }
                }
            }
        }
    }

    // MARK: - Cancel ongoing transfer

    /// Request cancellation of the ongoing send or pull.  The C++ engine only
    /// sets a flag; the transfer stops at the next chunk boundary and reports
    /// back via FT_EVT_TRANSFER_CANCELLED (send) / return code -2 or -8.
    func cancelTransfer() {
        guard isBusy, let engine = handle else { return }
        ft_cancel_transfer(engine)
        appendLog("Cancel requested — waiting for the current chunk to finish…")
    }

    /// Delete partial files left on the peer by cancelled sends (CMD_DELETE,
    /// matched by the original file name on the peer side).
    func deletePartialFile() {
        guard !pendingPartialNames.isEmpty else { return }
        let names = pendingPartialNames
        pendingPartialNames = []
        guard isConnected, let engine = handle else {
            appendLog("ERROR: Cannot delete partial: not connected")
            pendingPartialNames = names
            return
        }
        disconnectToken &+= 1
        inflightCalls += 1
        appendLog("Deleting \(names.count) partial file(s) on peer")
        Task.detached { [weak self, engine, names] in
            // Immutable snapshot: a captured var would be a Swift 6 sendability error.
            let failed = names.filter { ft_delete_file(engine, $0) != 0 }
            await MainActor.run { [weak self] in
                guard let self else { return }
                self.endEngineCall()
                if failed.isEmpty {
                    self.transferStatus = "Partial deleted"
                    self.appendLog("Partial deleted: \(names.joined(separator: ", "))")
                    self.scheduleAutoDisconnect(after: 2.0)
                } else {
                    self.pendingPartialNames = failed
                    self.appendLog("ERROR: Delete partial failed: \(failed.joined(separator: ", "))")
                }
            }
        }
    }
}

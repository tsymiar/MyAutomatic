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
        performSend(sendQueue.removeFirst())
    }

    private func performSend(_ filePath: String) {
        guard isConnected, let engine = handle else {
            transferStatus = "Not connected"
            return
        }

        let url = URL(fileURLWithPath: filePath)
        let fileName = url.lastPathComponent
        let fileSize = (try? url.resourceValues(forKeys: [.fileSizeKey]).fileSize).map(UInt64.init) ?? 0

        isBusy = true
        transferStatus = "Preparing..."
        transferFileName = fileName
        transferredBytes = 0; totalBytes = fileSize; progress = 0
        appendLog("Sending: \(fileName) (\(formatBytes(fileSize)))")

        let idx = transferTasks.count
        transferTasks.append(TransferTask(
            fileName: fileName, fileSize: fileSize,
            direction: .sending, peerIP: "Peer Device"
        ))
        transferTasks[idx].status = .transferring

        Task.detached { [weak self, engine] in
            let ret = ft_send_file(engine, filePath)
            await MainActor.run { [weak self] in
                guard let self, idx < self.transferTasks.count else { return }
                self.isBusy = false
                let task = self.transferTasks[idx]
                if ret == 0 {
                    self.transferStatus = "Send complete"; self.progress = 1.0
                    self.appendLog("Send complete: \(task.fileName)")
                    self.transferTasks[idx].status = .completed
                    self.transferTasks[idx].bytesTransferred = task.fileSize
                } else if ret == -2 {
                    // FT_EVT_TRANSFER_CANCELLED callback already marked the task
                    // .cancelled — do not overwrite with .failed
                    self.transferStatus = "Send cancelled"
                    self.appendLog("Send cancelled: \(task.fileName)")
                    // Peer keeps the partial file; arm the delete button
                    self.pendingDeleteFile = task.fileName
                } else {
                    self.transferStatus = "Send failed (code \(ret))"
                    self.appendLog("Send failed: \(task.fileName) (code \(ret))")
                    self.transferTasks[idx].status = .failed
                }
                // Next queued file; auto-disconnect only after the queue is done.
                if !self.sendQueue.isEmpty {
                    self.drainSendQueue()
                } else if ret == 0 && self.isConnected && self.pendingDeleteFile == nil {
                    // Auto-disconnect only after success: a cancelled send keeps the
                    // connection so the delete button can send CMD_DELETE. Wait 1s so
                    // the peer finishes processing the final CMD_COMPLETE.
                    DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) { [weak self] in
                        self?.disconnect()
                    }
                }
            }
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

        isBusy = true
        transferStatus = "Requesting \(name)…"
        transferredBytes = 0; totalBytes = 0; progress = 0
        appendLog("Pulling from \(host): \(name)")

        Task.detached { [weak self, engine, host, port, name] in
            let ret = ft_request_file(engine, host, port, name)
            await MainActor.run { [weak self] in
                guard let self else { return }
                self.isBusy = false
                if ret == 0 {
                    self.transferStatus = "Pull complete"
                    self.appendLog("Pull complete: \(name)")
                    // Status + on-disk path handled by the FT_EVT_TRANSFER_COMPLETE callback
                } else if ret == -8 {
                    // Cancelled (locally or by peer): the FT_EVT_TRANSFER_CANCELLED
                    // callback already marked the task .cancelled
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

    /// Delete the partial file left on the peer by a cancelled send
    /// (CMD_DELETE, matched by the original file name on the peer side).
    /// Disconnects 1s after success — the whole cancel flow is then done.
    func deletePartialFile() {
        guard let name = pendingDeleteFile else { return }
        pendingDeleteFile = nil
        guard isConnected, let engine = handle else {
            appendLog("ERROR: Cannot delete partial: not connected")
            return
        }
        appendLog("Deleting partial on peer: \(name)")
        Task.detached { [weak self, engine, name] in
            let ret = ft_delete_file(engine, name)
            await MainActor.run { [weak self] in
                guard let self else { return }
                if ret == 0 {
                    self.transferStatus = "Partial deleted: \(name)"
                    // Wait 1s so the peer finishes handling CMD_DELETE, then close
                    DispatchQueue.main.asyncAfter(deadline: .now() + 2.0) { [weak self] in
                        self?.disconnect()
                    }
                } else {
                    self.appendLog("ERROR: Delete partial failed: \(name) (code \(ret))")
                }
            }
        }
    }
}

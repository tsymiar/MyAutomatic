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
        transferredBytes = 0; totalBytes = fileSize; progress = 0
        appendLog("Sending: \(fileName) (\(formatBytes(fileSize)))")

        let idx = transferTasks.count
        transferTasks.append(TransferTask(
            fileName: fileName, fileSize: fileSize,
            direction: .sending, peerIP: "Peer Device"
        ))

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
                } else {
                    self.transferStatus = "Send failed (code \(ret))"
                    self.appendLog("Send failed: \(task.fileName) (code \(ret))")
                    self.transferTasks[idx].status = .failed
                }
                // Keep going with the next queued file; only disconnect once
                // the whole queue is done.
                if !self.sendQueue.isEmpty {
                    self.drainSendQueue()
                } else if self.isConnected {
                    // Wait 1s so the peer receives the final CMD_COMPLETE and
                    // finishes processing it
                    DispatchQueue.main.asyncAfter(deadline: .now() + 1.0) { [weak self] in
                        self?.disconnect()
                    }
                }
            }
        }
    }
}

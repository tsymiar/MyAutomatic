import Foundation
import FileTransferCore

// ──────────────────────────────────────────────────────────────────────────────
//  TransferCore — C callback wiring and progress handling
//  Both trampolines pass self to C as userData via Unmanaged, so no global
//  state is needed.
// ──────────────────────────────────────────────────────────────────────────────

@MainActor
extension TransferCore {

    // MARK: - Callback wiring (uses Unmanaged to avoid global state)

    /// Called by makeEngine() in +Engine, so it cannot be private.
    func installCallbacks(_ engine: FT_Handle) {
        // ── Progress callback ──
        let progTrampoline: FT_ProgressCallback = { rawSelf, cur, tot, stPtr in
            let status = stPtr.map { String(cString: $0) } ?? ""
            let core = Unmanaged<TransferCore>.fromOpaque(rawSelf!).takeUnretainedValue()
            DispatchQueue.main.async {
                core.handleProgress(current: cur, total: tot, status: status)
            }
        }
        callbackRef = progTrampoline
        ft_set_progress_callback(engine, progTrampoline, Unmanaged.passUnretained(self).toOpaque())

        // ── Log callback (global — routes ALL C++ LOG_* to the UI console) ──
        let logTrampoline: FT_LogCallback = { rawSelf, msgPtr in
            let msg = msgPtr.map { String(cString: $0) } ?? ""
            let core = Unmanaged<TransferCore>.fromOpaque(rawSelf!).takeUnretainedValue()
            DispatchQueue.main.async {
                core.appendTimestampedLog(msg)
            }
        }
        logCallbackRef = logTrampoline
        ft_set_log_callback(logTrampoline, Unmanaged.passUnretained(self).toOpaque())
    }

    /// Called on the main thread by the trampoline above.
    private func handleProgress(current: UInt64, total: UInt64, status: String) {
        transferredBytes = current
        totalBytes = total
        transferStatus = status
        progress = total > 0 ? Double(current) / Double(total) : 0

        let lower = status.lowercased()
        let isKeyEvent = lower.contains("connected") || lower.contains("disconnect")
            || lower.contains("complete") || lower.contains("cancel")
            || lower.contains("listening") || lower.contains("failed") || lower.contains("error")

        if isKeyEvent {
            appendLog(status)
        }

        // Update the first active task
        if let idx = transferTasks.firstIndex(where: { $0.status == .pending || $0.status == .transferring }) {
            transferTasks[idx].status = .transferring
            transferTasks[idx].bytesTransferred = current
            if lower.contains("complete!") {
                transferTasks[idx].status = .completed
            } else if lower.contains("cancel") {
                transferTasks[idx].status = .cancelled
            }
        }

        // Detect new incoming files (server mode)
        if isServerRunning, status.contains("Receiving:") {
            let name = status.components(separatedBy: ": ").last ?? ""
            if !transferTasks.contains(where: { $0.fileName == name && $0.direction == .receiving }) {
                transferTasks.append(TransferTask(
                    fileName: name, fileSize: total,
                    direction: .receiving, peerIP: "Peer"
                ))
            }
        }

        // Append completed received files
        // After "Transfer complete!" the C++ side appends the real path on disk
        // behind a "|": the received file name carries a timestamp suffix, so a
        // path rebuilt here from the original name would not exist.
        if status.localizedCaseInsensitiveContains("complete!") {
            let parts = status.components(separatedBy: "|")
            let pathHint = (parts.count == 2 && !parts[1].isEmpty) ? parts[1] : nil

            for task in transferTasks where task.status == .completed && task.direction == .receiving {
                let url = pathHint.map { URL(fileURLWithPath: $0) }
                    ?? URL(fileURLWithPath: savePath).appendingPathComponent(task.fileName)
                guard !receivedFiles.contains(where: { $0.savedPath == url }) else { continue }
                receivedFiles.append(ReceivedFile(
                    fileName: url.lastPathComponent, fileSize: task.fileSize,
                    fromIP: task.peerIP, savedPath: url, receivedAt: Date()
                ))
            }
        }
    }
}

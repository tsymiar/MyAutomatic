import Foundation
import FileTransferCore

// The FT_EVT_* anonymous C enum imports as Int; widen once so every
// comparison below uses UInt32 (the FT_ProgressCallback event type).
private let evtNone         = UInt32(FT_EVT_NONE)
private let evtReceiveStart = UInt32(FT_EVT_RECEIVE_START)
private let evtComplete     = UInt32(FT_EVT_TRANSFER_COMPLETE)
private let evtCancelled    = UInt32(FT_EVT_TRANSFER_CANCELLED)

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
        let progTrampoline: FT_ProgressCallback = { rawSelf, cur, tot, stPtr, event in
            let status = stPtr.map { String(cString: $0) } ?? ""
            let core = Unmanaged<TransferCore>.fromOpaque(rawSelf!).takeUnretainedValue()
            DispatchQueue.main.async {
                core.handleProgress(current: cur, total: tot, status: status, event: event)
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
    /// `event` is the C-side FT_EVT_* code; event types are decided by the
    /// code, never by parsing the status string.
    private func handleProgress(current: UInt64, total: UInt64, status: String, event: UInt8) {
        transferredBytes = current
        totalBytes = total
        transferStatus = status
        progress = total > 0 ? Double(current) / Double(total) : 0

        let evt = UInt32(event)
        let isKeyEvent = evt != evtNone

        if isKeyEvent {
            appendLog(status)
        }

        // Update the first active task
        if let idx = transferTasks.firstIndex(where: { $0.status == .pending || $0.status == .transferring }) {
            transferTasks[idx].bytesTransferred = current
            switch evt {
            case evtComplete:
                transferTasks[idx].status = .completed
            case evtCancelled:
                transferTasks[idx].status = .cancelled
            default:
                transferTasks[idx].status = .transferring
            }
        }

        // Detect new incoming files: both server push and client pull emit this
        // event; total is the real file size.
        if evt == evtReceiveStart {
            let name = status.components(separatedBy: ": ").last ?? ""
            transferFileName = name
            if !transferTasks.contains(where: { $0.fileName == name && $0.direction == .receiving }) {
                transferTasks.append(TransferTask(
                    fileName: name, fileSize: total,
                    direction: .receiving, peerIP: "Peer"
                ))
            }
        }

        // Append completed received files; the C++ side appends the real on-disk
        // path behind "|" (the saved name carries a timestamp suffix).
        if evt == evtComplete {
            let parts = status.components(separatedBy: "|")
            let pathHint = (parts.count == 2 && !parts[1].isEmpty) ? parts[1] : nil
            if let pathHint {
                transferFileName = URL(fileURLWithPath: pathHint).lastPathComponent
            }

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

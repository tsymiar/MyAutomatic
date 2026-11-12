import Foundation
import FileTransferCore

// FT_EVT_* C enum imports as Int; widen once to UInt32 (the callback event type).
private let evtNone          = UInt32(FT_EVT_NONE)
private let evtSessionOpened = UInt32(FT_EVT_SESSION_OPENED)
private let evtSessionClosed = UInt32(FT_EVT_SESSION_CLOSED)
private let evtReceiveStart  = UInt32(FT_EVT_RECEIVE_START)
private let evtComplete      = UInt32(FT_EVT_TRANSFER_COMPLETE)
private let evtCancelled     = UInt32(FT_EVT_TRANSFER_CANCELLED)

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
        let progTrampoline: FT_ProgressCallback = { rawSelf, cur, tot, stPtr, event in
            let status = stPtr.map { String(cString: $0) } ?? ""
            let core = Unmanaged<TransferCore>.fromOpaque(rawSelf!).takeUnretainedValue()
            DispatchQueue.main.async {
                core.handleProgress(current: cur, total: tot, status: status, event: event)
            }
        }
        callbackRef = progTrampoline
        ft_set_progress_callback(engine, progTrampoline, Unmanaged.passUnretained(self).toOpaque())

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
        let evt = UInt32(event)

        // Session events only refresh the device list; they must not overwrite the status bar or progress.
        guard evt != evtSessionOpened, evt != evtSessionClosed else {
            refreshDevices()
            return
        }

        updateProgressState(current: current, total: total, status: status)
        updateActiveTask(evt: evt, current: current)
        if evt == evtReceiveStart { beginReceiveTask(total: total, status: status) }
        if evt == evtComplete || evt == evtCancelled { finishTerminalTask(evt: evt, status: status) }
    }

    /// Index of the task created by the transfer currently in flight, if any.
    private func activeTaskIndex() -> Int? {
        inFlightTaskIDs.last.flatMap { id in transferTasks.firstIndex { $0.id == id } }
    }

    /// Write @Published only on change so each callback doesn't recompute the whole view tree.
    private func updateProgressState(current: UInt64, total: UInt64, status: String) {
        if transferredBytes != current { transferredBytes = current }
        if totalBytes != total { totalBytes = total }
        if transferStatus != status { transferStatus = status }
        let newProgress = total > 0 ? Double(current) / Double(total) : 0
        if progress != newProgress { progress = newProgress }
    }

    /// Bind progress/terminal events only to the task this transfer created.
    private func updateActiveTask(evt: UInt32, current: UInt64) {
        guard let idx = activeTaskIndex() else { return }
        if transferTasks[idx].bytesTransferred != current {
            transferTasks[idx].bytesTransferred = current
        }
        switch evt {
        case evtComplete:
            transferTasks[idx].status = .completed
        case evtCancelled:
            transferTasks[idx].status = .cancelled
        case evtNone:
            if transferTasks[idx].status == .pending {
                transferTasks[idx].status = .transferring
            }
        default:
            break
        }
    }

    /// Receive task: both a server push and a client pull emit this event; total is the real file size.
    private func beginReceiveTask(total: UInt64, status: String) {
        let name = receiveFileName(status)
        transferFileName = name
        var task = TransferTask(
            fileName: name, fileSize: total,
            direction: .receiving, peerIP: peerAddress(status)
        )
        task.status = .transferring
        transferTasks.append(task)
        inFlightTaskIDs.append(task.id)
    }

    /// Complete/cancelled event: C++ appends the real on-disk path after "|"; duplicate names may carry a -N suffix.
    private func finishTerminalTask(evt: UInt32, status: String) {
        let parts = status.split(separator: "|", maxSplits: 1, omittingEmptySubsequences: false)
        let pathHint = parts.count == 2 && !parts[1].isEmpty ? String(parts[1]) : nil
        if let pathHint {
            transferFileName = URL(fileURLWithPath: pathHint).lastPathComponent
        }

        if evt == evtComplete, let idx = activeTaskIndex(),
           transferTasks[idx].direction == .receiving, let pathHint {
            recordReceivedFile(transferTasks[idx], savedAt: pathHint)
        }
        if let activeID = inFlightTaskIDs.last { inFlightTaskIDs.removeAll { $0 == activeID } }
    }

    /// Only the current receive task produces a record; historical tasks are untouched.
    private func recordReceivedFile(_ task: TransferTask, savedAt pathHint: String) {
        let url = URL(fileURLWithPath: pathHint)
        guard !receivedFiles.contains(where: { $0.savedPath == url }) else { return }
        receivedFiles.append(ReceivedFile(
            fileName: url.lastPathComponent, fileSize: task.fileSize,
            fromIP: task.peerIP, savedPath: url, receivedAt: Date()
        ))
    }

    /// Extract the "[ip:port] " prefix emitted by the server for peer events.
    private func peerAddress(_ status: String) -> String {
        guard status.hasPrefix("["), let close = status.firstIndex(of: "]") else { return "" }
        return String(status[status.index(after: status.startIndex)..<close])
    }

    private func receiveFileName(_ status: String) -> String {
        var s = status
        if s.hasPrefix("["), let close = s.firstIndex(of: "]") {
            s = String(s[s.index(after: close)...]).trimmingCharacters(in: .whitespaces)
        }
        for prefix in ["Receiving: ", "Pulling: "] where s.hasPrefix(prefix) {
            return String(s.dropFirst(prefix.count))
        }
        return s
    }
}

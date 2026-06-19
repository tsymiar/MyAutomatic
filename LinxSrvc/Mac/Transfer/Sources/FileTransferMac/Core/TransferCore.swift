import Foundation
import FileTransferCore

// ──────────────────────────────────────────────────────────────────────────────
//  TransferCore — thin Swift wrapper around the C++ TransferEngine (via C bridge)
//  @MainActor ensures all @Published mutations happen on the main thread.
//  Uses Unmanaged<TransferCore> as the callback userData to avoid global state.
// ──────────────────────────────────────────────────────────────────────────────

@MainActor
final class TransferCore: ObservableObject {

    // MARK: - Published state

    @Published var isServerRunning = false
    @Published var isConnected = false
    @Published var clientCount = 0
    @Published var serverIP = TransferCore.resolveLocalIP()

    // Client-mode text field state (persists across tab switches)
    @Published var targetIP = ""
    @Published var targetPort = "8800"

    @Published var transferStatus: String = ""
    @Published var transferredBytes: UInt64 = 0
    @Published var totalBytes: UInt64 = 0
    @Published var progress: Double = 0           // 0.0 … 1.0

    @Published var isBusy = false                 // true while sending

    /// Directory for received files.
    @Published var savePath: String = {
        let dl = FileManager.default.homeDirectoryForCurrentUser
            .appendingPathComponent("Downloads")
            .appendingPathComponent("FileTransfer")
        try? FileManager.default.createDirectory(at: dl, withIntermediateDirectories: true)
        return dl.path
    }()

    // MARK: - Internal state

    /// Opaque C++ handle (TransferEngine*)
    private var handle: FT_Handle?

    /// Keep a strong reference to the C callback so it is not deallocated.
    private var callbackRef: FT_ProgressCallback?

    // View models
    @Published var receivedFiles: [ReceivedFile] = []
    @Published var transferTasks: [TransferTask] = []

    /// Console-style log messages (capped at 200 entries).
    @Published var logMessages: [String] = []

    // MARK: - Lifetime

    deinit {
        if let h = handle { ft_destroy(h); handle = nil }
    }

    // MARK: - Logging helper

    private func appendLog(_ msg: String) {
        let line = "[\(timeFormatter.string(from: Date()))] \(msg)"
        logMessages.append(line)
        if logMessages.count > 200 {
            logMessages.removeFirst(50)
        }
    }

    // MARK: - Engine lifecycle

    /// Create engine, set save path & callback. Returns the handle on success, nil on failure.
    private func makeEngine() -> FT_Handle? {
        let h = ft_create()
        guard let h else {
            transferStatus = "Failed to create instance"
            appendLog("ERROR: Failed to create TransferEngine instance")
            return nil
        }
        ft_set_save_path(h, savePath)
        installCallback(h)
        return h
    }

    /// Tear down the engine handle cleanly.
    private func destroyEngine() {
        guard let h = handle else { return }
        ft_destroy(h)
        handle = nil
        callbackRef = nil
    }

    func startServer(port: UInt16 = 8800) {
        guard !isServerRunning, let h = makeEngine() else { return }
        handle = h

        let rc = ft_start_server(h, port)
        if rc == 0 {
            isServerRunning = true
            transferStatus = "Listening on port \(port)"
            appendLog("Server started on port \(port)")
        } else {
            transferStatus = "Server start failed (code \(rc))"
            appendLog("ERROR: Server start failed (code \(rc))")
            destroyEngine()
        }
    }

    func stopServer() {
        guard isServerRunning else { return }
        ft_stop_server(handle)
        isServerRunning = false
        clientCount = 0
        transferStatus = "Server stopped"
        appendLog("Server stopped")
        destroyEngine()
    }

    /// Poll client count periodically (server mode).
    func updateClientCount() {
        guard let h = handle, isServerRunning else { return }
        clientCount = Int(ft_get_client_count(h))
    }

    // MARK: - Client mode

    func connect(to ip: String, port: UInt16 = 8800) {
        guard !isConnected, let h = makeEngine() else { return }
        handle = h

        let rc = ft_connect(h, ip, port)
        if rc == 0 {
            isConnected = true
            transferStatus = "Connected to \(ip):\(port)"
            appendLog("Connected to \(ip):\(port)")
        } else {
            transferStatus = "Connection failed (code \(rc))"
            appendLog("ERROR: Connection to \(ip):\(port) failed (code \(rc))")
            destroyEngine()
        }
    }

    func disconnect() {
        guard isConnected else { return }
        ft_disconnect(handle)
        isConnected = false
        transferStatus = "Disconnected"
        appendLog("Disconnected from server")
        destroyEngine()
    }

    // MARK: - Send file (client mode, blocking → run on background)

    func sendLocalFile(_ filePath: String) {
        guard isConnected, let h = handle else {
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
        transferTasks.append(TransferTask(fileName: fileName, fileSize: fileSize, direction: .sending, peerIP: "Peer Device"))

        Task.detached { [weak self, h] in
            let rc = ft_send_file(h, filePath)
            await MainActor.run { [weak self] in
                guard let self, idx < self.transferTasks.count else { return }
                self.isBusy = false
                let task = self.transferTasks[idx]
                if rc == 0 {
                    self.transferStatus = "Send complete"; self.progress = 1.0
                    self.appendLog("Send complete: \(task.fileName)")
                    self.transferTasks[idx].status = .completed
                    self.transferTasks[idx].bytesTransferred = task.fileSize
                } else {
                    self.transferStatus = "Send failed (code \(rc))"
                    self.appendLog("Send failed: \(task.fileName) (code \(rc))")
                    self.transferTasks[idx].status = .failed
                }
            }
        }
    }

    // MARK: - Callback wiring (uses Unmanaged to avoid global state)

    private func installCallback(_ h: FT_Handle) {
        let trampoline: FT_ProgressCallback = { rawSelf, cur, tot, stPtr in
            let status = stPtr.map { String(cString: $0) } ?? ""
            let core = Unmanaged<TransferCore>.fromOpaque(rawSelf!).takeUnretainedValue()
            DispatchQueue.main.async {
                core.handleProgress(current: cur, total: tot, status: status)
            }
        }
        callbackRef = trampoline
        ft_set_progress_callback(h, trampoline, Unmanaged.passUnretained(self).toOpaque())
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
            if lower.contains("complete!") { transferTasks[idx].status = .completed }
            else if lower.contains("cancel") { transferTasks[idx].status = .cancelled }
        }

        // Detect new incoming files (server mode)
        if isServerRunning, status.contains("Receiving:") {
            let name = status.components(separatedBy: ": ").last ?? ""
            if !transferTasks.contains(where: { $0.fileName == name && $0.direction == .receiving }) {
                transferTasks.append(TransferTask(fileName: name, fileSize: total, direction: .receiving, peerIP: "Peer"))
            }
        }

        // Append completed received files
        if status.localizedCaseInsensitiveContains("complete!") {
            for task in transferTasks where task.status == .completed && task.direction == .receiving
                && !receivedFiles.contains(where: { $0.fileName == task.fileName }) {
                let path = URL(fileURLWithPath: savePath).appendingPathComponent(task.fileName)
                receivedFiles.append(ReceivedFile(
                    fileName: task.fileName, fileSize: task.fileSize,
                    fromIP: task.peerIP, savedPath: path, receivedAt: Date()
                ))
            }
        }
    }

    // MARK: - Progress reset

    /// Reset progress state — called when switching to Receive tab
    /// so the previous send-completion bar doesn't linger.
    func resetProgress() {
        transferredBytes = 0
        totalBytes = 0
        progress = 0
        isBusy = false
        transferStatus = ""
    }

    // MARK: - Utilities

    /// Resolve the primary local IPv4 address (en0 / Ethernet / WiFi).
    static func resolveLocalIP() -> String {
        var addr = "127.0.0.1"
        var ifaddr: UnsafeMutablePointer<ifaddrs>?
        guard getifaddrs(&ifaddr) == 0, let first = ifaddr else { return addr }
        defer { freeifaddrs(ifaddr) }

        for ptr in sequence(first: first, next: { $0.pointee.ifa_next }) {
            let flags = Int32(ptr.pointee.ifa_flags)
            let name = String(cString: ptr.pointee.ifa_name)
            let family = ptr.pointee.ifa_addr.pointee.sa_family

            guard family == UInt8(AF_INET),
                  (flags & IFF_UP) != 0,
                  !name.hasPrefix("lo"),
                  !name.hasPrefix("utun"),
                  !name.hasPrefix("llw") else { continue }

            var host = [CChar](repeating: 0, count: Int(NI_MAXHOST))
            if getnameinfo(ptr.pointee.ifa_addr, socklen_t(ptr.pointee.ifa_addr.pointee.sa_len),
                           &host, socklen_t(host.count), nil, 0, NI_NUMERICHOST) == 0 {
                addr = String(cString: host)
                if name == "en0" { break }
            }
        }
        return addr
    }
}

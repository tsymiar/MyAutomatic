import Foundation
import FileTransferCore

// ──────────────────────────────────────────────────────────────────────────────
//  TransferCore — thin Swift wrapper around the C++ TransferEngine (via C bridge)
//  @MainActor ensures all @Published mutations happen on the main thread.
//  Uses Unmanaged<TransferCore> as the callback userData to avoid global state.
//
//  This file holds state + small helpers only; behaviour lives in the sibling
//  extension files in the same directory, grouped by topic:
//    • TransferCore+Engine.swift     engine lifecycle, server / client connection
//    • TransferCore+Sending.swift    send queue and single-file sending
//    • TransferCore+Callbacks.swift  C callback wiring and progress handling
//  Members touched from those files cannot be private (Swift private is
//  file-scoped), hence the internal storage below.
// ──────────────────────────────────────────────────────────────────────────────

@MainActor
final class TransferCore: ObservableObject {

    // MARK: - Published state

    @Published var isServerRunning = false
    @Published var isConnected = false
    @Published var devices: [ConnectedDevice] = []
    @Published var serverIP = TransferCore.resolveLocalIP()

    // Client-mode text field state (persists across tab switches)
    @Published var targetIP = ""
    @Published var targetPort: String = UserDefaults.standard.string(forKey: "defaultPort") ?? "8800"

    /// Default port pre-filled into the Client port field. Persisted and applied immediately.
    @Published var defaultPort: String = UserDefaults.standard.string(forKey: "defaultPort") ?? "8800" {
        didSet {
            UserDefaults.standard.set(defaultPort, forKey: "defaultPort")
            targetPort = defaultPort
        }
    }

    /// Whether the bottom log console is shown (default true). Persisted.
    /// object(forKey:) is used instead of bool(forKey:) so the default is true, not false.
    @Published var showLogConsole: Bool = UserDefaults.standard.object(forKey: "showLogConsole") as? Bool ?? true {
        didSet { UserDefaults.standard.set(showLogConsole, forKey: "showLogConsole") }
    }

    @Published var transferStatus: String = ""
    @Published var transferFileName = ""          // file shown on the progress bar
    @Published var transferredBytes: UInt64 = 0
    @Published var totalBytes: UInt64 = 0
    @Published var progress: Double = 0           // 0.0 … 1.0

    @Published var isBusy = false                 // true while sending
    @Published var isConnecting = false           // true while ft_connect is in flight
    @Published var pendingPartialNames: [String] = []  // cancelled sends awaiting partial-file deletion

    /// ENOTCONN retry policy: exponential backoff vs. short grace then close.
    /// Persisted in UserDefaults; pushed to the engine on change and on every new engine.
    @Published var backoffEnabled: Bool = UserDefaults.standard.bool(forKey: "backoffEnabled") {
        didSet {
            UserDefaults.standard.set(backoffEnabled, forKey: "backoffEnabled")
            if let engine = handle { ft_set_backoff_enabled(engine, backoffEnabled) }
        }
    }

    /// Directory for received files; changing it updates the running engine too. Persisted.
    @Published var savePath: String = {
        let saved = UserDefaults.standard.string(forKey: "savePath")
        let uri = saved.map { URL(fileURLWithPath: $0) }
            ?? FileManager.default.homeDirectoryForCurrentUser
                .appendingPathComponent("Downloads")
                .appendingPathComponent("FileTransfer")
        try? FileManager.default.createDirectory(at: uri, withIntermediateDirectories: true)
        return uri.path
    }() {
        didSet {
            do {
                try FileManager.default.createDirectory(atPath: savePath,
                                                        withIntermediateDirectories: true)
            } catch {
                appendLog("ERROR: Cannot create receive directory \(savePath): \(error.localizedDescription)")
            }
            UserDefaults.standard.set(savePath, forKey: "savePath")
            if let engine = handle { ft_set_save_path(engine, savePath) }
        }
    }

    // MARK: - Internal state

    /// Opaque C++ handle (TransferEngine*) — internal so the +Engine / +Callbacks
    /// extensions can reach it.
    var handle: FT_Handle?

    /// Keep a strong reference to the C callback so it is not deallocated.
    var callbackRef: FT_ProgressCallback?
    var logCallbackRef: FT_LogCallback?

    @Published var receivedFiles: [ReceivedFile] = []
    @Published var transferTasks: [TransferTask] = []

    /// Console-style log messages (capped at 200 entries).
    @Published var logMessages: [LogEntry] = []

    /// Queue of files waiting to be sent. Multiple files must go one after
    /// another: sending two files at once over a single connection interleaves
    /// their chunks, while the server writes one file per session in order —
    /// the result on disk would be a mix of both.
    /// Consumed by TransferCore+Sending.swift (internal for that reason).
    var sendQueue: [String] = []

    // MARK: - Lifetime

    /// Tasks currently transferring; completion events bind only to these, never replay history.
    var inFlightTaskIDs: [UUID] = []

    /// Detached C calls holding the raw engine pointer; teardown waits for them.
    var inflightCalls = 0
    /// Teardown requested while calls were in flight.
    var pendingTeardown = false
    /// Invalidates a pending auto-disconnect timer when a new transfer starts.
    var disconnectToken: UInt64 = 0

    deinit {
        // ~TransferEngine emits logs; clear both callbacks before destroying so
        // no C log can reach a deinitialising self.
        ft_set_log_callback(nil, nil)
        if let engine = handle {
            ft_set_progress_callback(engine, nil, nil)
            ft_destroy(engine)
        }
    }

    // MARK: - Logging helper

    func appendLog(_ msg: String) {
        appendLine("[\(timeFormatter.string(from: Date()))] \(msg)")
    }

    /// Append a log line that already has a timestamp prefix (e.g. from C++ LOG_* macros).
    /// Avoids double-timestamping.
    func appendTimestampedLog(_ msg: String) {
        appendLine(msg)
    }

    private func appendLine(_ line: String) {
        guard logMessages.last?.text != line else { return }
        logMessages.append(LogEntry(text: line))
        if logMessages.count > 200 {
            logMessages.removeFirst(50)
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
        transferFileName = ""
        transferStatus = ""
    }

    /// Reset the progress bar only when no client send/pull is in flight.
    /// Server start/stop must not clobber the progress of a concurrent client
    /// transfer now that both roles share one engine; isBusy also guards the
    /// disconnect button, so it is never touched here.
    func resetProgressIfIdle() {
        guard !isBusy else { return }
        transferredBytes = 0
        totalBytes = 0
        progress = 0
        transferFileName = ""
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
            guard let ifaName = ptr.pointee.ifa_name, let sa = ptr.pointee.ifa_addr else { continue }
            let name = String(cString: ifaName)

            guard sa.pointee.sa_family == UInt8(AF_INET),
                  (flags & Int32(IFF_UP)) != 0,
                  !name.hasPrefix("lo"),
                  !name.hasPrefix("utun"),
                  !name.hasPrefix("llw") else { continue }

            var host = [CChar](repeating: 0, count: Int(NI_MAXHOST))
            // Use the struct size instead of BSD-only sa_len so this compiles for both IPv4 and IPv6.
            let saLen = socklen_t(MemoryLayout<sockaddr_in>.size)
            if getnameinfo(sa, saLen, &host, socklen_t(host.count), nil, 0, NI_NUMERICHOST) == 0 {
                addr = String(cString: host)
                if name == "en0" { break }
            }
        }
        return addr
    }
}

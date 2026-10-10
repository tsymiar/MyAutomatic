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
    @Published var clientCount = 0
    @Published var serverIP = TransferCore.resolveLocalIP()

    // Client-mode text field state (persists across tab switches)
    @Published var targetIP = ""
    @Published var targetPort: String = UserDefaults.standard.string(forKey: "defaultPort") ?? "8800"

    /// Default port pre-filled into the Server / Client port fields. Persisted.
    @Published var defaultPort: String = UserDefaults.standard.string(forKey: "defaultPort") ?? "8800" {
        didSet { UserDefaults.standard.set(defaultPort, forKey: "defaultPort") }
    }

    /// Whether the bottom log console is shown (default true). Persisted.
    /// object(forKey:) is used instead of bool(forKey:) so the default is true, not false.
    @Published var showLogConsole: Bool = UserDefaults.standard.object(forKey: "showLogConsole") as? Bool ?? true {
        didSet { UserDefaults.standard.set(showLogConsole, forKey: "showLogConsole") }
    }

    @Published var transferStatus: String = ""
    @Published var transferredBytes: UInt64 = 0
    @Published var totalBytes: UInt64 = 0
    @Published var progress: Double = 0           // 0.0 … 1.0

    @Published var isBusy = false                 // true while sending

    /// ENOTCONN retry policy: exponential backoff (legacy) vs. short grace then close.
    /// Persisted in UserDefaults; pushed to the engine on change and on every new engine.
    @Published var backoffEnabled: Bool = UserDefaults.standard.bool(forKey: "backoffEnabled") {
        didSet {
            UserDefaults.standard.set(backoffEnabled, forKey: "backoffEnabled")
            if let engine = handle { ft_set_backoff_enabled(engine, backoffEnabled) }
        }
    }

    /// Directory for received files.
    @Published var savePath: String = {
        let uri = FileManager.default.homeDirectoryForCurrentUser
            .appendingPathComponent("Downloads")
            .appendingPathComponent("FileTransfer")
        try? FileManager.default.createDirectory(at: uri, withIntermediateDirectories: true)
        return uri.path
    }()

    // MARK: - Internal state

    /// Opaque C++ handle (TransferEngine*) — internal so the +Engine / +Callbacks
    /// extensions can reach it.
    var handle: FT_Handle?

    /// Keep a strong reference to the C callback so it is not deallocated.
    var callbackRef: FT_ProgressCallback?
    var logCallbackRef: FT_LogCallback?

    // View models
    @Published var receivedFiles: [ReceivedFile] = []
    @Published var transferTasks: [TransferTask] = []

    /// Console-style log messages (capped at 200 entries).
    @Published var logMessages: [String] = []

    /// Queue of files waiting to be sent. Multiple files must go one after
    /// another: sending two files at once over a single connection interleaves
    /// their chunks, while the server writes one file per session in order —
    /// the result on disk would be a mix of both.
    /// Consumed by TransferCore+Sending.swift (internal for that reason).
    var sendQueue: [String] = []

    // MARK: - Lifetime

    deinit {
        if let engine = handle { ft_destroy(engine); handle = nil }
    }

    // MARK: - Logging helper

    func appendLog(_ msg: String) {
        let line = "[\(timeFormatter.string(from: Date()))] \(msg)"
        logMessages.append(line)
        if logMessages.count > 200 {
            logMessages.removeFirst(50)
        }
    }

    /// Append a log line that already has a timestamp prefix (e.g. from C++ LOG_* macros).
    /// Avoids double-timestamping.
    func appendTimestampedLog(_ msg: String) {
        logMessages.append(msg)
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

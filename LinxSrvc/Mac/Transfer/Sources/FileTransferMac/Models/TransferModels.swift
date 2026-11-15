import Foundation

/// Per-role status text shared by the Server and Client tabs. One `@Published`
/// value keeps both tabs' state together while each role stays logically distinct.
struct RoleStatus {
    var server: String = ""   // Server tab: server lifecycle + receive progress
    var client: String = ""   // Client tab: connection + send/receive progress
}

/// Transfer task model
struct TransferTask: Identifiable, Equatable {
    let id = UUID()
    let fileName: String
    let fileSize: UInt64
    let direction: Direction
    let peerIP: String
    var bytesTransferred: UInt64 = 0
    var status: TransferStatus = .pending
    var startTime: Date = Date()

    enum Direction: String { case sending, receiving }
    enum TransferStatus: String {
        case pending, transferring, completed, failed, cancelled
    }

    var progress: Double {
        fileSize > 0 ? Double(bytesTransferred) / Double(fileSize) : 0
    }

    var formattedProgress: String {
        String(format: "%.1f%%", progress * 100)
    }

    var formattedSpeed: String {
        let elapsed = Date().timeIntervalSince(startTime)
        guard elapsed > 0, bytesTransferred > 0 else { return "--" }
        let bps = Double(bytesTransferred) / elapsed
        if bps > 1_000_000 { return String(format: "%.1f MB/s", bps / 1_000_000) }
        if bps > 1_000 { return String(format: "%.1f KB/s", bps / 1_000) }
        return String(format: "%.0f B/s", bps)
    }

    var formattedSize: String {
        if fileSize > 1_000_000_000 { return String(format: "%.2f GB", Double(fileSize) / 1_000_000_000) }
        if fileSize > 1_000_000 { return String(format: "%.2f MB", Double(fileSize) / 1_000_000) }
        if fileSize > 1_000 { return String(format: "%.2f KB", Double(fileSize) / 1_000) }
        return "\(fileSize) B"
    }

    // No custom ==: SwiftUI diffs fields; an id-only == would hide changed-progress rows.
}

struct ReceivedFile: Identifiable {
    let id = UUID()
    let fileName: String
    let fileSize: UInt64
    let fromIP: String
    let savedPath: URL
    let receivedAt: Date
}

/// A peer discovered on the LAN via the discovery beacon.
struct DiscoveredServer: Identifiable, Equatable {
    // Different ports on one IP are distinct sessions; using ip alone would collide row ids.
    var id: String { "\(ip):\(port)" }
    let ip: String
    let port: UInt16
    var lastSeen: Date
}

/// Server-side device snapshot. A short-lived connection stays listed as
/// offline so the device remains visible after the socket closes.
struct ConnectedDevice: Identifiable, Equatable {
    // Different ports on one IP are distinct sessions; using ip alone would collide row ids.
    var id: String { "\(ip):\(port)" }
    let ip: String
    let port: UInt16
    let online: Bool
    let lastSeen: Date
    // Live receive state from the engine snapshot; an empty file name means idle.
    let transferringFile: String
    let transferred: UInt64
    let total: UInt64
}

/// Log line with a stable identity: trimming the buffer must not shift row ids.
struct LogEntry: Identifiable {
    let id = UUID()
    let text: String
}

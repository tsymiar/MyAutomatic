import Foundation
import FileTransferCore

// ──────────────────────────────────────────────────────────────────────────────
//  TransferCore — LAN discovery (server beacon broadcast is automatic once the
//  server starts; this file covers the client-side listener that parses peer
//  beacons so the user can send files without typing an IP).
// ──────────────────────────────────────────────────────────────────────────────

@MainActor
extension TransferCore {

    // MARK: - Peer discovery (client side)

    /// Start the peer beacon listener. Ensures an engine exists (reusing the one a
    /// running server/client already holds), then listens for UDP beacons and
    /// populates `discoveredServers`. Safe to call when already active.
    func startPeerDiscovery() {
        guard discoverPeers, !peerDiscoveryActive else { return }

        let engine: FT_Handle
        if let existing = handle {
            engine = existing
        } else {
            guard let created = makeEngine() else { return }
            engine = created
            handle = created
        }
        ft_set_discovery_enabled(engine, broadcastEnabled)

        if discoveryCallbackRef == nil {
            discoveryCallbackRef = discoveryTrampoline
        }
        peerDiscoveryActive = true
        let started = ft_start_discovery(engine, discoveryCallbackRef,
                                         Unmanaged.passUnretained(self).toOpaque())
        if started {
            appendLog("LAN discovery listener started")
        } else {
            peerDiscoveryActive = false
            appendLog("ERROR: LAN discovery listener failed to bind udp/8801 (port in use?)")
        }
    }

    /// Stop the peer beacon listener and release the engine if nothing else needs it.
    func stopPeerDiscovery() {
        guard peerDiscoveryActive else { return }
        if let engine = handle {
            ft_stop_discovery(engine)
        }
        peerDiscoveryActive = false
        discoveredServers.removeAll()
        appendLog("LAN discovery listener stopped")
        destroyEngineIfIdle()
    }

    /// Marshalled from the C discovery trampoline; runs on the main actor.
    func handleDiscoveredServer(ip: String, port: UInt16) {
        // Ignore the beacon from this app's own server (it loops back with our LAN
        // IP). Filter by our server port, not by IP alone, so other processes on
        // the same host (e.g. the CLI test tool) are still discoverable.
        if isServerRunning, ip == TransferCore.resolveLocalIP(), port == activeServerPort {
            return
        }
        let id = "\(ip):\(port)"
        if let idx = discoveredServers.firstIndex(where: { $0.id == id }) {
            discoveredServers[idx].lastSeen = Date()
        } else {
            discoveredServers.append(DiscoveredServer(ip: ip, port: port, lastSeen: Date()))
        }
    }

    /// Drop peers not seen in the last 15s (the beacon repeats every 3s).
    func pruneDiscoveredServers() {
        guard !discoveredServers.isEmpty else { return }
        let cutoff = Date().addingTimeInterval(-15)
        let local = TransferCore.resolveLocalIP()
        let ownPort = isServerRunning ? activeServerPort : 0
        let next = discoveredServers.filter {
            guard $0.lastSeen >= cutoff else { return false }
            // Drop our own server's loopback beacon, but keep other local peers.
            return !(ownPort != 0 && $0.port == ownPort && $0.ip == local)
        }
        if next != discoveredServers { discoveredServers = next }
    }
}

/// C callback trampoline: extracts the TransferCore instance from userData and
/// forwards the discovered peer to the main actor. Kept at file scope so it has a
/// stable function address for the C ABI.
private func discoveryTrampoline(_ userData: UnsafeMutableRawPointer?,
                                 _ ip: UnsafePointer<CChar>?,
                                 _ port: UInt16) {
    guard let userData, let ip else { return }
    let core = Unmanaged<TransferCore>.fromOpaque(userData).takeUnretainedValue()
    let ipStr = String(cString: ip)
    Task { @MainActor in
        core.handleDiscoveredServer(ip: ipStr, port: port)
    }
}

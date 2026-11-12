import Foundation
import FileTransferCore

// ──────────────────────────────────────────────────────────────────────────────
//  TransferCore — engine lifecycle and connection (server / client)
//  Extracted from TransferCore.swift so no single class body grows too large.
// ──────────────────────────────────────────────────────────────────────────────

@MainActor
extension TransferCore {

    // MARK: - Engine lifecycle

    /// Create engine, set save path & callback. Returns the handle on success, nil on failure.
    private func makeEngine() -> FT_Handle? {
        let engine = ft_create()
        guard let engine else {
            transferStatus = "Failed to create instance"
            appendLog("ERROR: Failed to create TransferEngine instance")
            return nil
        }
        ft_set_save_path(engine, savePath)
        ft_set_backoff_enabled(engine, backoffEnabled)
        installCallbacks(engine)
        return engine
    }

    /// Tear down the engine handle cleanly. Defers while detached C calls still hold it.
    private func destroyEngine() {
        guard handle != nil else { return }
        guard inflightCalls == 0 else {
            pendingTeardown = true
            return
        }
        pendingTeardown = false
        guard let engine = handle else { return }
        // ~TransferEngine emits a log; clear callbacks first so it cannot reach a stale self.
        ft_set_progress_callback(engine, nil, nil)
        ft_set_log_callback(nil, nil)
        ft_destroy(engine)
        handle = nil
        callbackRef = nil
        logCallbackRef = nil
    }

    /// Called on the main actor when a detached C call finishes.
    func endEngineCall() {
        inflightCalls = max(0, inflightCalls - 1)
        if inflightCalls == 0 && pendingTeardown { destroyEngine() }
    }

    /// Destroy the engine only when neither role needs it. One engine instance
    /// can listen and connect at the same time, so stopping one side must keep
    /// the engine alive while the other side is still active.
    private func destroyEngineIfIdle() {
        guard !isServerRunning, !isConnected else { return }
        destroyEngine()
    }

    /// Schedule an auto-disconnect that is cancelled if another transfer starts first.
    func scheduleAutoDisconnect(after seconds: Double) {
        disconnectToken &+= 1
        let token = disconnectToken
        DispatchQueue.main.asyncAfter(deadline: .now() + seconds) { [weak self] in
            guard let self, self.disconnectToken == token, !self.isBusy else { return }
            self.disconnect()
        }
    }

    // MARK: - Server mode

    func startServer(port: UInt16 = 8800) {
        guard !isServerRunning else { return }
        // Reuse the engine a connected client is already using; create one only when absent.
        let engine: FT_Handle
        if let existing = handle {
            engine = existing
        } else {
            guard let created = makeEngine() else { return }
            engine = created
            handle = created
        }

        let ret = ft_start_server(engine, port)
        if ret == 0 {
            devices = []
            resetProgressIfIdle()
            isServerRunning = true
            transferStatus = "Listening on port \(port)"
            appendLog("Server started on port \(port)")
        } else {
            transferStatus = "Server start failed (code \(ret))"
            appendLog("ERROR: Server start failed (code \(ret))")
            destroyEngineIfIdle()
        }
    }

    func stopServer() {
        guard isServerRunning else { return }
        ft_stop_server(handle)
        isServerRunning = false
        devices = []
        resetProgressIfIdle()
        transferStatus = "Server stopped"
        appendLog(transferStatus)
        destroyEngineIfIdle()
    }

    /// Reconcile the device list with the engine. The engine keeps recent
    /// connections after the socket closes, so short-lived peers stay visible.
    func refreshDevices() {
        guard let engine = handle, isServerRunning else { return }
        // Allocate by the engine's current entry count; a fixed 32 would hide devices past the 32nd.
        let capacity = max(1, Int(ft_get_client_count(engine)))
        var buffer = [FT_ClientInfo](repeating: FT_ClientInfo(), count: capacity)
        let count = buffer.withUnsafeMutableBufferPointer { ptr in
            Int(ft_get_clients(engine, ptr.baseAddress, Int32(ptr.count)))
        }
        guard count > 0 else {
            if !devices.isEmpty { devices = [] }
            return
        }

        var snapshot: [ConnectedDevice] = []
        snapshot.reserveCapacity(count)
        for i in 0..<count {
            var ipTuple = buffer[i].ip
            let ipSize = MemoryLayout.size(ofValue: ipTuple)
            let ip = withUnsafePointer(to: &ipTuple) { ptr in
                ptr.withMemoryRebound(to: CChar.self, capacity: ipSize) { String(cString: $0) }
            }
            guard !ip.isEmpty else { continue }
            let info = buffer[i]
            var fileTuple = info.file
            let fileSize = MemoryLayout.size(ofValue: fileTuple)
            let file = withUnsafePointer(to: &fileTuple) { ptr in
                ptr.withMemoryRebound(to: CChar.self, capacity: fileSize) { String(cString: $0) }
            }
            snapshot.append(ConnectedDevice(
                ip: ip, port: info.port, online: info.online,
                lastSeen: Date(timeIntervalSince1970: Double(info.lastSeenMs) / 1000.0),
                transferringFile: file,
                transferred: info.transferred, total: info.total
            ))
        }
        if devices != snapshot { devices = snapshot }
    }

    // MARK: - Client mode

    func connect(to host: String, port: UInt16 = 8800) {
        guard !isConnected, !isConnecting else { return }
        // Reuse the engine a running server is already using; create one only when absent.
        let engine: FT_Handle
        if let existing = handle {
            engine = existing
        } else {
            guard let created = makeEngine() else { return }
            engine = created
            handle = created
        }
        isConnecting = true
        transferStatus = "Connecting to \(host):\(port)…"
        appendLog("Connecting to \(host):\(port)")

        // ft_connect has a 3s connect timeout; keep it off the main thread.
        inflightCalls += 1
        Task.detached { [weak self, engine, host, port] in
            let ret = ft_connect(engine, host, port)
            await MainActor.run { [weak self] in
                guard let self else { return }
                self.endEngineCall()
                self.isConnecting = false
                if ret == 0 {
                    self.isConnected = true
                    self.resetProgress()
                    self.transferStatus = "Connected to \(host):\(port)"
                    self.appendLog("Connected to \(host):\(port)")
                } else {
                    self.transferStatus = "Connection failed (code \(ret))"
                    self.appendLog("ERROR: Connection to \(host):\(port) failed (code \(ret))")
                    // A running server keeps the shared engine; only an idle engine is destroyed.
                    self.destroyEngineIfIdle()
                }
            }
        }
    }

    func disconnect() {
        guard isConnected || isConnecting else { return }
        ft_disconnect(handle)
        isConnected = false
        transferStatus = "Disconnected"
        appendLog("Disconnected from server")
        resetProgress()
        destroyEngineIfIdle()
    }
}

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

    /// Tear down the engine handle cleanly.
    private func destroyEngine() {
        guard let engine = handle else { return }
        ft_destroy(engine)
        handle = nil
        callbackRef = nil
        // Note: log callback is global and survives engine destruction;
        // explicitly clear it so no stale Unmanaged pointer is used.
        ft_set_log_callback(nil, nil)
        logCallbackRef = nil
    }

    // MARK: - Server mode

    func startServer(port: UInt16 = 8800) {
        guard !isServerRunning, let engine = makeEngine() else { return }
        handle = engine

        let ret = ft_start_server(engine, port)
        if ret == 0 {
            isServerRunning = true
            transferStatus = "Listening on port \(port)"
            appendLog("Server started on port \(port)")
        } else {
            transferStatus = "Server start failed (code \(ret))"
            appendLog("ERROR: Server start failed (code \(ret))")
            destroyEngine()
        }
    }

    func stopServer() {
        guard isServerRunning else { return }
        ft_stop_server(handle)
        isServerRunning = false
        clientCount = 0
        transferStatus = "Server stopped by swift"
        appendLog(transferStatus)
        destroyEngine()
    }

    /// Poll client count periodically (server mode).
    func updateClientCount() {
        guard let engine = handle, isServerRunning else { return }
        clientCount = Int(ft_get_client_count(engine))
    }

    // MARK: - Client mode

    func connect(to host: String, port: UInt16 = 8800) {
        guard !isConnected, let engine = makeEngine() else { return }
        handle = engine

        // Reset the progress bar so it does not keep the previous transfer's 100%
        resetProgress()

        let ret = ft_connect(engine, host, port)
        if ret == 0 {
            isConnected = true
            transferStatus = "Connected to \(host):\(port)"
            appendLog("Connected to \(host):\(port)")
        } else {
            transferStatus = "Connection failed (code \(ret))"
            appendLog("ERROR: Connection to \(host):\(port) failed (code \(ret))")
            destroyEngine()
        }
    }

    func disconnect() {
        guard isConnected else { return }
        ft_disconnect(handle)
        isConnected = false
        transferStatus = "Disconnected"
        appendLog("Disconnected from server")
        resetProgress()
        destroyEngine()
    }
}

// TransferBridge.h — C bridging API for Swift interop
// Wraps TransferEngine (C++) behind opaque handle + C function pointers.
// Callbacks carry a userData pointer so Swift can pass an instance reference
// without relying on global state.

#ifndef TransferBridge_h
#define TransferBridge_h

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Opaque handle to a TransferEngine instance.
typedef void* FT_Handle;

/// Progress callback signature.
/// Called from arbitrary threads; the receiver must dispatch to main thread if needed.
///   userData – opaque pointer registered via ft_set_progress_callback()
///   current  – bytes transferred so far
///   total    – total file size in bytes
///   status   – human-readable status string (UTF-8, null-terminated)
///   event    – FT_EVT_* code; 0 (FT_EVT_NONE) means a routine progress update
typedef void (*FT_ProgressCallback)(void* userData, uint64_t current, uint64_t total,
                                    const char* status, uint8_t event);

/// Event codes carried in the `event` argument of FT_ProgressCallback.
/// Mirrors the FT_EVT_* enum in TransferEngine.h.
enum {
    FT_EVT_NONE               = 0,  ///< routine progress update
    FT_EVT_CONNECTED          = 1,  ///< peer connected
    FT_EVT_DISCONNECTED       = 2,  ///< connection closed
    FT_EVT_RECEIVE_START      = 3,  ///< new incoming transfer request
    FT_EVT_TRANSFER_COMPLETE  = 4,  ///< transfer finished
    FT_EVT_TRANSFER_CANCELLED = 5,  ///< transfer cancelled
    FT_EVT_REQUEST_PULL       = 6   ///< incoming file pull request (CMD_REQUEST)
};

// ── Lifetime ──────────────────────────────────────────

/// Create a new transfer instance.  Returns NULL on allocation failure.
FT_Handle ft_create(void);

/// Destroy the instance and release all resources (stops server / disconnects).
void ft_destroy(FT_Handle handle);

// ── Server mode ───────────────────────────────────────

/// Start listening on `port` (default 8800).  Returns 0 on success, <0 on error.
int ft_start_server(FT_Handle handle, uint16_t port);

/// Stop listening and disconnect all clients.
void ft_stop_server(FT_Handle handle);

// ── Client mode ───────────────────────────────────────

/// Connect to `ip`:`port`.  Returns 0 on success, <0 on error.
int ft_connect(FT_Handle handle, const char* ip, uint16_t port);

/// Disconnect from the server.
void ft_disconnect(FT_Handle handle);

// ── File operations ───────────────────────────────────

/// Send a file (client mode, must be connected).  Blocks until transfer completes or fails.
/// Returns 0 on success, <0 on error.
int ft_send_file(FT_Handle handle, const char* filePath);

/// Request a file from a remote server (pull mode).  Connects to ip:port and
/// blocks until the file has been received into the save path or fails.
/// The peer only serves regular files from its own save path; a missing file
/// is rejected with return code -5.
/// Returns 0 on success, <0 on error.
int ft_request_file(FT_Handle handle, const char* ip, uint16_t port, const char* fileName);

/// Request cancellation of the ongoing transfer (send or pull).  Takes effect
/// at the next chunk boundary; a cancelled send notifies the peer with
/// CMD_CANCEL so it stops receiving and treats the file as cancelled.
void ft_cancel_transfer(FT_Handle handle);

/// Ask the peer to delete the partial file left by a previously cancelled
/// send, matched by the original file name.  The peer only removes files it
/// recorded when handling CMD_CANCEL; unknown names are ignored.
/// Returns 0 on success, <0 on error.
int ft_delete_file(FT_Handle handle, const char* fileName);

// ── Configuration ─────────────────────────────────────

/// Set the directory where received files are saved.
void ft_set_save_path(FT_Handle handle, const char* path);

/// Set the retry policy for connections that died before any data arrived (ENOTCONN).
///   true  – exponential backoff: up to 16 retries, delay doubles from 10ms (~655s total).
///   false – short fixed grace: 3 retries of 20ms, then the connection is closed (default).
void ft_set_backoff_enabled(FT_Handle handle, bool enabled);

/// Register a progress callback with an opaque user-data pointer.
/// The `userData` pointer is forwarded verbatim to every invocation of `callback`.
/// Pass callback==NULL to clear.
void ft_set_progress_callback(FT_Handle handle, FT_ProgressCallback callback, void* userData);

/// Log callback signature — receives every LOG_INF/WRN/ERR/DBG from C++.
///   userData – opaque pointer registered via ft_set_log_callback()
///   msg      – formatted log line (UTF-8, null-terminated, includes timestamp)
typedef void (*FT_LogCallback)(void* userData, const char* msg);

/// Register a global log callback.  When set, ALL C++ logs (from any TransferEngine
/// instance) are forwarded to `callback` in addition to being printed to stdout.
/// Pass callback==NULL to disable forwarding.
void ft_set_log_callback(FT_LogCallback callback, void* userData);

// ── Status queries ────────────────────────────────────

bool ft_is_connected(FT_Handle handle);
bool ft_is_server_running(FT_Handle handle);
int ft_get_client_count(FT_Handle handle);

#ifdef __cplusplus
}
#endif

#endif /* TransferBridge_h */

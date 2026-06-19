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
typedef void (*FT_ProgressCallback)(void* userData, uint64_t current, uint64_t total, const char* status);

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

// ── Configuration ─────────────────────────────────────

/// Set the directory where received files are saved.
void ft_set_save_path(FT_Handle handle, const char* path);

/// Register a progress callback with an opaque user-data pointer.
/// The `userData` pointer is forwarded verbatim to every invocation of `callback`.
/// Pass callback==NULL to clear.
void ft_set_progress_callback(FT_Handle handle, FT_ProgressCallback callback, void* userData);

// ── Status queries ────────────────────────────────────

bool ft_is_connected(FT_Handle handle);
bool ft_is_server_running(FT_Handle handle);
int ft_get_client_count(FT_Handle handle);

#ifdef __cplusplus
}
#endif

#endif /* TransferBridge_h */

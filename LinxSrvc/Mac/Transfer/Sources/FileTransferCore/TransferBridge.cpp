// TransferBridge.cpp — C wrapper around TransferEngine (C++)

#include "TransferBridge.h"
#include "TransferEngine.h"
#include <string>

// ---------------------------------------------------------------------------
//  Lifetime
// ---------------------------------------------------------------------------

FT_Handle ft_create(void) {
    return static_cast<FT_Handle>(new TransferEngine());
}

void ft_destroy(FT_Handle handle) {
    if (!handle) return;
    delete static_cast<TransferEngine*>(handle);
}

// ---------------------------------------------------------------------------
//  Server mode
// ---------------------------------------------------------------------------

int ft_start_server(FT_Handle handle, uint16_t port) {
    if (!handle) return -1;
    return static_cast<TransferEngine*>(handle)->startServer(port);
}

void ft_stop_server(FT_Handle handle) {
    if (!handle) return;
    static_cast<TransferEngine*>(handle)->stopServer();
}

// ---------------------------------------------------------------------------
//  Client mode
// ---------------------------------------------------------------------------

int ft_connect(FT_Handle handle, const char* ip, uint16_t port) {
    if (!handle) return -1;
    return static_cast<TransferEngine*>(handle)->connectToServer(std::string(ip ? ip : ""), port);
}

void ft_disconnect(FT_Handle handle) {
    if (!handle) return;
    static_cast<TransferEngine*>(handle)->disconnect();
}

// ---------------------------------------------------------------------------
//  File operations
// ---------------------------------------------------------------------------

int ft_send_file(FT_Handle handle, const char* filePath) {
    if (!handle) return -1;
    return static_cast<TransferEngine*>(handle)->sendLocalFile(std::string(filePath ? filePath : ""));
}

// ---------------------------------------------------------------------------
//  Configuration
// ---------------------------------------------------------------------------

void ft_set_save_path(FT_Handle handle, const char* path) {
    if (!handle) return;
    static_cast<TransferEngine*>(handle)->setSavePath(std::string(path ? path : ""));
}

void ft_set_progress_callback(FT_Handle handle, FT_ProgressCallback callback, void* userData) {
    if (!handle) return;

    if (callback) {
        // Capture both the C function pointer and userData in the std::function
        // so the C++ core can call it from any thread.
        static_cast<TransferEngine*>(handle)->setProgressCallback(
            [callback, userData](uint64_t current, uint64_t total, const std::string& status) {
                callback(userData, current, total, status.c_str());
            });
    } else {
        static_cast<TransferEngine*>(handle)->setProgressCallback(nullptr);
    }
}

// ---------------------------------------------------------------------------
//  Status queries
// ---------------------------------------------------------------------------

bool ft_is_connected(FT_Handle handle) {
    if (!handle) return false;
    return static_cast<TransferEngine*>(handle)->isConnected();
}

bool ft_is_server_running(FT_Handle handle) {
    if (!handle) return false;
    return static_cast<TransferEngine*>(handle)->isServerRunning();
}

int ft_get_client_count(FT_Handle handle) {
    if (!handle) return 0;
    return static_cast<TransferEngine*>(handle)->getClientCount();
}

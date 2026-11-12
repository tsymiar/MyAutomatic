// TransferBridge.cpp — C wrapper around TransferEngine (C++)

#include "TransferBridge.h"
#include "TransferEngine.h"
#include "CommLogger.h"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <string>
#include <mutex>
#include <new>

// Log callback storage (global, because LOG_* macros are also global).
// Replace cb + userData as one unit, or a concurrent logger could pair a new cb with stale userData.
struct LogSink {
    FT_LogCallback cb;
    void* userData;
};
static std::mutex g_logMutex;
static LogSink g_logSink{nullptr, nullptr};

/// Trampoline called by CommLogger.h's g_logOutputFunc → invokes the C callback.
static void logTrampoline(const char* msg)
{
    LogSink sink;
    {
        std::lock_guard<std::mutex> lock(g_logMutex);
        sink = g_logSink;
    }
    if (sink.cb) {
        sink.cb(sink.userData, msg);
    }
}

// --------------------------------------------------------------------------
//  Lifetime
// --------------------------------------------------------------------------
FT_Handle ft_create(void) {
    // No exception may cross the C ABI; allocation failure returns NULL as documented.
    return static_cast<FT_Handle>(new (std::nothrow) TransferEngine());
}

void ft_destroy(FT_Handle handle) {
    if (!handle) return;
    delete static_cast<TransferEngine*>(handle);
}

// --------------------------------------------------------------------------
//  Server mode
// --------------------------------------------------------------------------
int ft_start_server(FT_Handle handle, uint16_t port) {
    if (!handle) return -1;
    return static_cast<TransferEngine*>(handle)->startServer(port);
}

void ft_stop_server(FT_Handle handle) {
    if (!handle) return;
    static_cast<TransferEngine*>(handle)->closeServer();
}

// --------------------------------------------------------------------------
//  Client mode
// --------------------------------------------------------------------------
int ft_connect(FT_Handle handle, const char* ip, uint16_t port) {
    if (!handle) return -1;
    return static_cast<TransferEngine*>(handle)->connectToServer(std::string(ip ? ip : ""), port);
}

void ft_disconnect(FT_Handle handle) {
    if (!handle) return;
    static_cast<TransferEngine*>(handle)->disconnect();
}

int ft_send_file(FT_Handle handle, const char* filePath) {
    if (!handle) return -1;
    return static_cast<TransferEngine*>(handle)->postLocalFile(std::string(filePath ? filePath : ""));
}

// --------------------------------------------------------------------------
//  File operations
// --------------------------------------------------------------------------
int ft_request_file(FT_Handle handle, const char* ip, uint16_t port, const char* fileName) {
    if (!handle) return -1;
    return static_cast<TransferEngine*>(handle)->requestFile(
        std::string(ip ? ip : ""), port, std::string(fileName ? fileName : ""));
}

void ft_cancel_transfer(FT_Handle handle) {
    if (!handle) return;
    static_cast<TransferEngine*>(handle)->cancelTransfer();
}

int ft_delete_file(FT_Handle handle, const char* fileName) {
    if (!handle) return -1;
    return static_cast<TransferEngine*>(handle)->deletePartialFile(
        std::string(fileName ? fileName : ""));
}

// --------------------------------------------------------------------------
//  Configuration
// --------------------------------------------------------------------------
void ft_set_save_path(FT_Handle handle, const char* path) {
    if (!handle) return;
    static_cast<TransferEngine*>(handle)->setSavePath(std::string(path ? path : ""));
}

void ft_set_backoff_enabled(FT_Handle handle, bool enabled) {
    if (!handle) return;
    static_cast<TransferEngine*>(handle)->setBackoffEnabled(enabled);
}

void ft_set_progress_callback(FT_Handle handle, FT_ProgressCallback callback, void* userData) {
    if (!handle) return;

    if (callback) {
        // Capture the C fn pointer + userData so the core can invoke it from any thread.
        static_cast<TransferEngine*>(handle)->setProgressCallback(
            [callback, userData](uint64_t current, uint64_t total,
                                 const std::string& status, uint8_t event) {
                callback(userData, current, total, status.c_str(), event);
            });
    } else {
        static_cast<TransferEngine*>(handle)->setProgressCallback(nullptr);
    }
}

// --------------------------------------------------------------------------
//  Log callback (global — affects all LOG_* macros everywhere)
// --------------------------------------------------------------------------
void ft_set_log_callback(FT_LogCallback callback, void* userData)
{
    {
        std::lock_guard<std::mutex> lock(g_logMutex);
        g_logSink = {callback, userData};
    }
    g_logOutputFunc.store(callback ? logTrampoline : nullptr, std::memory_order_release);
}

bool ft_is_connected(FT_Handle handle) {
    if (!handle) return false;
    return static_cast<TransferEngine*>(handle)->isConnected();
}

bool ft_is_server_running(FT_Handle handle) {
    if (!handle) return false;
    return static_cast<TransferEngine*>(handle)->isServerRunning();
}

// --------------------------------------------------------------------------
//  Status queries
// --------------------------------------------------------------------------
int ft_get_client_count(FT_Handle handle) {
    if (!handle) return 0;
    return static_cast<TransferEngine*>(handle)->getClientCount();
}

int ft_get_clients(FT_Handle handle, FT_ClientInfo* out, int capacity) {
    if (!handle || !out || capacity <= 0) return 0;
    std::vector<ClientInfo> list = static_cast<TransferEngine*>(handle)->getClients();
    int n = std::min<int>((int)list.size(), capacity);
    for (int i = 0; i < n; ++i) {
        FT_ClientInfo& dst = out[i];
        memset(&dst, 0, sizeof(dst));
        snprintf(dst.ip, sizeof(dst.ip), "%s", list[i].ip.c_str());
        dst.port = list[i].port;
        dst.online = list[i].online;
        dst.lastSeenMs = list[i].lastSeenMs;
        snprintf(dst.file, sizeof(dst.file), "%s", list[i].transferringFile.c_str());
        dst.transferred = list[i].transferredBytes;
        dst.total = list[i].totalBytes;
    }
    return n;
}

#include "WsServe.h"
#include "Utils.h"

#ifdef WITH_WEBSOCKETS

#include <libwebsockets.h>

#include <cstring>
#include <deque>
#include <string>
#include <thread>
#include <vector>

namespace {

constexpr size_t MAX_WS_MSG = 16 * 1024;   // max accepted inbound frame size

struct SessionData {
    std::deque<std::string>* out = nullptr;  // per-connection outbound queue
};

int wsCallback(struct lws* wsi, enum lws_callback_reasons reason,
               void* user, void* in, size_t len);
void WsBroadcast(const std::string& message);   // fwd (anon ns)

// single "echo" protocol; plain ws:// clients with no subprotocol match it
struct lws_protocols protocols[] = {
    { "echo", wsCallback, sizeof(SessionData), MAX_WS_MSG, 0, nullptr, 0 },
    { nullptr, nullptr, 0, 0, 0, nullptr, 0 }
};

std::vector<struct lws*> g_clients;        // all live connections (one thread)
WsMessageCallback g_onMessage = nullptr;   // user hook (optional)
std::thread g_wsThread;
volatile bool g_running = false;

int wsCallback(struct lws* wsi, enum lws_callback_reasons reason,
               void* user, void* in, size_t len)
{
    auto* sd = reinterpret_cast<SessionData*>(user);
    switch (reason) {
    case LWS_CALLBACK_ESTABLISHED:
        Message("ws: client connected");
        if (sd->out == nullptr)
            sd->out = new std::deque<std::string>();
        g_clients.push_back(wsi);
        break;

    case LWS_CALLBACK_RECEIVE: {
        std::string msg(static_cast<const char*>(in), len);
        Message("ws: recv %zu bytes", len);
        if (g_onMessage != nullptr) {
            g_onMessage(msg);              // user decides how to reply (e.g. broadcast)
        } else {
            WsBroadcast(msg);              // default behavior: echo to all
        }
        break;
    }

    case LWS_CALLBACK_SERVER_WRITEABLE:
        if (sd->out != nullptr && !sd->out->empty()) {
            const std::string& m = sd->out->front();
            unsigned char* buf = new unsigned char[LWS_PRE + m.size()];
            memcpy(buf + LWS_PRE, m.data(), m.size());
            int n = lws_write(wsi, buf + LWS_PRE, m.size(), LWS_WRITE_TEXT);
            delete[] buf;
            if (n < 0) {
                Error("ws: lws_write failed");
                return -1;
            }
            sd->out->pop_front();
            if (!sd->out->empty())
                lws_callback_on_writable(wsi);
        }
        break;

    case LWS_CALLBACK_CLOSED:
        Message("ws: client closed");
        for (auto it = g_clients.begin(); it != g_clients.end(); ++it) {
            if (*it == wsi) { g_clients.erase(it); break; }
        }
        if (sd->out != nullptr) { delete sd->out; sd->out = nullptr; }
        break;

    default:
        break;
    }
    return 0;
}

void WsBroadcast(const std::string& message)
{
    for (auto* wsi : g_clients) {
        auto* sd = reinterpret_cast<SessionData*>(lws_wsi_user(wsi));
        if (sd != nullptr && sd->out != nullptr) {
            sd->out->push_back(message);
            lws_callback_on_writable(wsi);
        }
    }
}

void wsServiceLoop(int port)
{
    struct lws_context_creation_info info;
    memset(&info, 0, sizeof(info));
    info.port = port;
    info.protocols = protocols;
    info.gid = -1;
    info.uid = -1;

    struct lws_context* context = lws_create_context(&info);
    if (context == nullptr) {
        Error("ws: create context failed on port %d", port);
        return;
    }
    Message("websocket server start over [%d] OK!", port);
    while (g_running) {
        lws_service(context, 50);   // 50ms poll; all lws ops stay on this thread
    }
    lws_context_destroy(context);
    Message("websocket server stopped");
}

} // namespace

void WsOnMessage(WsMessageCallback cb) { g_onMessage = cb; }

int WsStartServer(int port)
{
    // Port used to be short: >32767 wrapped to a negative value, lws failed to bind
    if (port <= 0 || port > 65535) {
        Error("ws: invalid port %d, expect 1~65535!", port);
        return -1;
    }
    if (g_running) {
        Warning("ws: server already running");
        return 0;
    }
    g_running = true;
    g_wsThread = std::thread(wsServiceLoop, port);
    return 0;
}

void WsStopServer()
{
    if (!g_running)
        return;
    g_running = false;
    if (g_wsThread.joinable())
        g_wsThread.join();
}

#endif // WITH_WEBSOCKETS

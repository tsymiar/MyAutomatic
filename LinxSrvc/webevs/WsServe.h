#pragma once
#include <string>

typedef void (*WsMessageCallback)(const std::string& message);

#ifdef WITH_WEBSOCKETS
// Start a standalone libwebsockets server on `port`. It runs in its own thread
// (its own lws event loop) so it can coexist with the existing evhttp HTTP
// server bound to a different port. Returns 0 on success, negative on failure.
int WsStartServer(short port);

// Best-effort stop and join of the websocket service thread.
void WsStopServer();

// Register a callback invoked on the WS thread for every received text frame.
// The handler may call WsBroadcast() to push data back to all clients.
void WsOnMessage(WsMessageCallback cb);

// Broadcast a text message to all currently connected clients. Safe to call
// from the WS service thread (e.g. from the registered callback).
void WsBroadcast(const std::string& message);
#else
// Stubs when libwebsockets is unavailable (WS server disabled at build time).
inline int WsStartServer(short) { return -1; }
inline void WsStopServer() {}
inline void WsOnMessage(WsMessageCallback) {}
inline void WsBroadcast(const std::string&) {}
#endif

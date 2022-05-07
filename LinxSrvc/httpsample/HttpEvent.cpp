#include "HttpEvent.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/queue.h>
#include <stdio.h>
#include <string.h>
#include <unordered_map>
#include <thread>

#include "event2/http_struct.h"
#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/dns.h"
#include "event2/thread.h"
#include "event2/keyvalq_struct.h" 
#include "event2/buffer_compat.h"

#include "Utils.h"

using namespace std;

vector<string> headList = {
    "server",
    "flag",
    "username",
    "token"
};

namespace {
    const int g_wait100ms = 100000;
    const char* HTTPD_SIGNATURE = "HttpEvent";
    static unordered_map<void*, string> g_msgRcvs = {};
    static unordered_map<string, string> g_extraOpts = {};
    static unordered_map<string, DealHooks> g_dealhooks = {};
}

void Response(struct evhttp_request* request, HookDetail detail = {});
void Release(event_base* base, evhttp_connection* evcon = nullptr);

void NullFunc(...)
{
    Message("calling null func");
}

void ElegantlyBreak(void* arg)
{
    if (arg != nullptr && !event_base_got_exit((struct event_base*)arg)) {
        event_base_loopexit((struct event_base*)arg, nullptr);
    }
}

DEALRES_CALLBACK GetResHook(string uri, evhttp_cmd_type cmd)
{
    auto it = g_dealhooks.find(uri);
    if (it != g_dealhooks.end() && it->second.method == cmd) {
        return it->second.callback;
    }
    return (DEALRES_CALLBACK)NullFunc;
}

void RemoteReadCallback(struct evhttp_request* remote_host, void* arg)
{
    if (remote_host == nullptr)
        return;
    Message("remote_rsp: %s[%d]", remote_host->remote_host, remote_host->remote_port);
    ElegantlyBreak(arg);
}
#ifdef evhttp_request_error
void RemoteRequestErrorCallback(enum evhttp_request_error error, void* arg)
{
    Error("request failed: %d!", error);
    ElegantlyBreak(arg);
}
#endif
void RemoteConnectionCloseCallback(struct evhttp_connection* connection, void* arg)
{
    Warning("remote connection closed!");
    ElegantlyBreak(arg);
}

int ReadHeaderDoneCallback(struct evhttp_request* remote_rsp, void*)
{
#ifdef evhttp_request_get_response_code_line
    Message("< HTTP/1.1 %d %s", evhttp_request_get_response_code(remote_rsp), evhttp_request_get_response_code_line(remote_rsp));
#endif
    struct evkeyvalq* headers = evhttp_request_get_input_headers(remote_rsp);
    struct evkeyval* header;
    TAILQ_FOREACH(header, headers, next) {
        Message("< %s: %s", header->key, header->value);
    }
    Message("< ");
    return 0;
}

void ReadChunkCallback(struct evhttp_request* resp, void* base)
{
    const int len = 4096;
    char data[len];
    struct evbuffer* evbuf = evhttp_request_get_input_buffer(resp);
    size_t size = 0;
    int n = 0;
    while ((n = evbuffer_remove(evbuf, data, len)) > 0) {
        fwrite(data, n, 1, stdout);
        size += n;
    }
    g_msgRcvs[base] = data;
    DEALRES_CALLBACK func = g_dealhooks["handleResponse"].callback;
    if (func != nullptr) {
        HookDetail detail;
        detail.payload = data;
        func(detail, size);
    }
    fwrite("\n", 1, 1, stdout);
}

void GenericHandler(struct evhttp_request* req_ptr, void* param)
{
    if (req_ptr == nullptr) return;
    char* address = nullptr; ev_uint16_t port = 0;
    evhttp_connection* conn = evhttp_request_get_connection(req_ptr);
    if (conn != nullptr) {
        evhttp_connection_get_peer(conn, &address, &port);
    }
    const char* req_uri = evhttp_request_get_uri(req_ptr);
    char* dec_uri = evhttp_decode_uri(req_uri);
    struct evkeyvalq head;
    evhttp_parse_query(dec_uri, &head);
    string url = dec_uri;
    free(dec_uri);
    evhttp_cmd_type method = evhttp_request_get_command(req_ptr);
    size_t size = EVBUFFER_LENGTH(req_ptr->input_buffer);
    char* payload = (char*)evbuffer_pullup(req_ptr->input_buffer, size);
    if (size > 0 && payload[size] != '\0') {
        payload[size] = '\0';
    }
    Message("%s %s\trequest from: %s:%d\n[ %s ]", GetMethodName(method), url.c_str(), address, port, payload);
    vector<string> list = parseUri(url);
    if (param != nullptr) {
        HookDetail message = {};
        message.url = url;
        message.method = method;
        message.payload = payload;
        if (list.size() > 1) {
            GetResHook(list[1], method)(message, size);
        } else {
            SrvCallbacks* callbacks = reinterpret_cast<SrvCallbacks*>(param);
            if (callbacks->ParsReq != nullptr)
                message = callbacks->ParsReq(list, message.method, payload);
            if (callbacks->PackRsp != nullptr)
                callbacks->PackRsp(message);
        }
        Response(req_ptr, message);
    } else {
        int filedes[2];
        if (pipe(filedes) < 0) {
            Error("failed to create a pipe!");
            return;
        }
        int status = 0;
        pid_t childpid = fork();
        if (childpid == 0) {
            if (list.size() > 0 && list[0] == "log") {
                status = HTTP_OK;
                for (auto it = headList.begin(); it != headList.end(); ++it) {
                    const char* ptr = evhttp_find_header(&head, it->c_str());
                    if (ptr == nullptr) continue;
                    string val(ptr);
                    size_t len = val.size();
                    Message("request param: %s = %s", it->c_str(), val.c_str());
                    if (*it == "server") {
                        string server(val);
                        uint32_t port = 0;
                        ssize_t pos = (server.empty() ? 0 : server.find(":"));
                        string ip = server.substr(0, pos);
                        string subport = server.substr(pos + 1, server.size() - 1);
                        if (isNum(subport)) {
                            port = atoi(subport.c_str());
                        }
                        if (pos <= 0) {
                            Error("can't find ':' in '%s'!", server.c_str());
                            status = HTTP_NOTIMPLEMENTED;
                            break;
                        }
                        Message("server parse = %s:%u", ip.c_str(), port);
                        char* post_data = (char*)EVBUFFER_DATA(req_ptr->input_buffer);
                        Message("request post_data = %s", post_data);
                    }
                    if (*it == "flag") {
                        Message("flag = %s", val.c_str());
                    }
                }
                if (list.size() > 1 && list[1] == "save") {
                    Message("cmd = %s", getVariable(url, "cmd").c_str());
                }
            } else {
                Message("no resource to deal.");
            }
            close(filedes[0]);
            write(filedes[1], &status, sizeof(int));
            exit(0);
        } else if (childpid > 0) {
            pid_t pid = 0;
            do {
                pid = waitpid(childpid, nullptr, WNOHANG);
            } while (pid == 0);
            close(filedes[1]);
            if (read(filedes[0], &status, sizeof(int)) < 0) {
                Error("failed to read status from filedes[0]");
            }
            HookDetail message = {};
            message.msg = "OK";
            message.method = method;
            message.status = status;
            Response(req_ptr, message);
            if (pid == childpid) {
                Message("successfully release child %d", pid);
            } else {
                Error("some error ocurred");
            }
        }
    }
}

void WaitMsgTask(event_base* base)
{
    unsigned int count = 0;
    while (base == nullptr || g_msgRcvs[base].empty()) {
        if (count > 3)
            break;
        usleep(g_wait100ms);
        Message("dispatch timeout %us to exit", count);
        count++;
    };
    if (base != nullptr) {
        ElegantlyBreak(base);
    } else {
        exit(0);
    }
}

int HttpClient(HookDetail& detail)
{
    struct event_base* base = event_base_new();
    if (base == nullptr) {
        Error("create event base failed!");
        return -1;
    }

    struct evhttp_uri* uri = evhttp_uri_parse(detail.url.c_str());
    if (uri == nullptr) {
        Error("parse url(%s) failed!", detail.url.c_str());
        Release(base);
        return -2;
    }
    const char* path = evhttp_uri_get_path(uri);
    if (path == nullptr || strlen(path) == 0) {
        detail.url = "/";
        path = "(null)";
    }

    struct evhttp_request* request = evhttp_request_new(RemoteReadCallback, base);
    // Fire off the request
    if (request == nullptr) {
        Error("evhttp_request_new() failed!");
        Release(base);
        return -3;
    }

    const char* host = evhttp_uri_get_host(uri);
    if (host == nullptr) {
        Error("get host from %s failed!", detail.url.c_str());
        Release(base);
        return -4;
    }
    int port = evhttp_uri_get_port(uri);
    if (port < 0) port = 80;

    Message("url: %s, host: %s, port: %d, path: %s.", detail.url.c_str(), host, port, path);

    struct evhttp_connection* connect = evhttp_connection_base_new(base, nullptr, host, port);
    if (connect == nullptr) {
        Error("create evhttp connection failed!");
        Release(base);
        return -5;
    }
    evhttp_connection_set_closecb(connect, RemoteConnectionCloseCallback, base);
    if (request->evcon != nullptr) evhttp_connection_set_timeout(request->evcon, 300);
    detail.base = base;
    Message("http success connects!");

    const char* user_agent = "Mozilla/5.0 (Macintosh;"
        " Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/15.0 Safari/605.1.15";

    struct evkeyvalq* output_headers = evhttp_request_get_output_headers(request);
    evhttp_add_header(output_headers, "Host", host);
    evhttp_add_header(output_headers, "Connection", "keep-alive");
    evhttp_add_header(output_headers, "User-Agent", user_agent);
    evhttp_add_header(output_headers, "Accept", "*/*");
    evhttp_add_header(output_headers, "Accept-Encoding", "gzip,deflate,br");
    evhttp_add_header(output_headers, "Accept-Language", "zh-CN,zh;q=0.8");
    evhttp_add_header(output_headers, "Cache-Control", "max-age=0");
    if (g_extraOpts.empty()) {
        for (auto head : g_extraOpts) {
            evhttp_add_header(output_headers, head.first.c_str(), head.second.c_str());
        }
    }

    if (detail.filename != nullptr) {
        char conbuf[256] = { 0 };
        sprintf(conbuf, "http://%s", host);
        evhttp_add_header(output_headers, "Origin", conbuf);
        sprintf(conbuf, "application/x-www-form-urlencoded;charset=utf-8");
        Message("setting '%s' form-data", detail.filename);

        evbuffer* output_buffer = evhttp_request_get_output_buffer(request);
        const char* boundary = "------WebKitFormBoundaryAu886z32WLCM1Fl0\r\n";
        int bndlen = strlen(boundary);
        evbuffer_add(output_buffer, boundary, bndlen);

        char bndbuf[256];
        sprintf(bndbuf, "Content-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\n", detail.filename);
        bndlen += strlen(bndbuf);
        evbuffer_add(output_buffer, bndbuf, strlen(bndbuf));

        sprintf(bndbuf, "Content-Type: application/octet-stream\r\n\r\n");
        bndlen += strlen(bndbuf);
        evbuffer_add(output_buffer, bndbuf, strlen(bndbuf));

        FILE* fd = fopen(detail.filename, "rb");
        char buf[1024];
        size_t s;
        size_t bts = 0;

        if (!fd) {
            Error("Open file '%s' error!", detail.filename);
            Release(base, connect);
            return -6;
        }
        while ((s = fread(buf, 1, sizeof(buf), fd)) > 0) {
            evbuffer_add(output_buffer, buf, s);
            bts += s;
        }

        sprintf(bndbuf, "\r\n------WebKitFormBoundaryAu886z32WLCM1Fl0--\r\n");
        bndlen += strlen(bndbuf);
        evbuffer_add(output_buffer, bndbuf, strlen(bndbuf));

        fclose(fd);
        evutil_snprintf(buf, sizeof(buf) - 1, "%lu", (unsigned long)bts + bndlen);
        evhttp_add_header(output_headers, "Content-Length", buf);
        sprintf(conbuf, "multipart/form-data;boundary=%s", boundary);
        evhttp_add_header(output_headers, "Content-Type", conbuf);
    }

#ifdef evhttp_request_set_header_cb
    evhttp_request_set_header_cb(request, ReadHeaderDoneCallback);
#endif
    evhttp_request_set_chunked_cb(request, ReadChunkCallback);
#ifdef RemoteRequestErrorCallback
    evhttp_request_set_error_cb(request, RemoteRequestErrorCallback);
#endif
    evhttp_make_request(connect, request, detail.method, detail.url.c_str());

    event_base_dispatch(base);
    Release(base, connect);
    Message("request finished");

    return 0;
}

int StartServer(short port, struct SrvCallbacks* callbacks)
{
    struct event_base* base = event_base_new();
    if (base == nullptr) {
        Error("create event base failed!");
        return -1;
    }

    struct evhttp* http = evhttp_new(base);
    if (http == nullptr) {
        Error("http server malloc failed!");
        return -1;
    }

    const char* addr = "0.0.0.0";
    int ret = evhttp_bind_socket(http, addr, port);
    if (ret != 0) {
        Error("http bind socket failed(%d): %d!", ret, port);
        return -1;
    }

    evhttp_set_allowed_methods(http, EVHTTP_REQ_GET | EVHTTP_REQ_POST | EVHTTP_REQ_HEAD
        | EVHTTP_REQ_OPTIONS | EVHTTP_REQ_PUT | EVHTTP_REQ_DELETE);
    evhttp_set_gencb(http, GenericHandler, callbacks);
    Message("Http server start over [%d] OK!", port);

    event_base_dispatch(base);
    evhttp_free(http);
    return 0;
}

int RequestClient(const char* url, HookDetail& detail, DEALRES_CALLBACK hook)
{
    int stat = -1;
    thread client([&stat, &detail](const char* url) {
        detail.url = url;
        stat = HttpClient(detail);
        }, url);
    if (hook != nullptr) {
        WaitMsgTask(detail.base);
        if (client.joinable())
            client.join();
    } else {
        g_dealhooks["handleResponse"].callback = hook;
        client.detach();
        usleep(g_wait100ms);
    }
    for (auto msg : g_msgRcvs) {
        if (!msg.second.empty()) {
            detail.msg = msg.second;
        }
    }
    return stat;
}

void Response(struct evhttp_request* request, HookDetail message)
{
    struct evbuffer* buf = evbuffer_new();
    if (buf == nullptr) {
        Error("failed to create response buffer!");
        return;
    }
    string content = "";
    if (message.method == EVHTTP_REQ_OPTIONS) {
        message.status = HTTP_OK;
    } else {
        if (message.status < HTTP_OK && message.status != 0) {
            message.status = HTTP_BADMETHOD;
        } else if (message.status != HTTP_NOTIMPLEMENTED) {
            const char* status = (message.status == HTTP_OK ? "true" : "false");
            if (message.status == 0) {
                message.status = HTTP_BADMETHOD;
            }
            content =
                "{  \"status\": [" + to_string(message.status) +
                ", " + string(status) + "]" +
                (message.status != HTTP_OK ? (",  \"message\": \"" + message.msg + "\"") : "") +
                "}";
        } else {
            content = message.msg;
        }
    }
    if (request == NULL) {
        Error("client request disaliable!");
        return;
    }
    evhttp_add_header(request->output_headers, "Server", HTTPD_SIGNATURE);
    evhttp_add_header(request->output_headers, "Access-Control-Allow-Origin", "*");
    evhttp_add_header(request->output_headers, "Access-Control-Allow-Method", "POST, GET, OPTIONS");
    evhttp_add_header(request->output_headers, "Access-Control-Allow-Headers", "X-PINGOTHER, Content-Type");
    evhttp_add_header(request->output_headers, "Access-Control-Max-Age", "1728000");
    evhttp_add_header(request->output_headers, "X-Xss-Protection", "1; mode=block");
    evhttp_add_header(request->output_headers, "Connection", "close");
    if (g_extraOpts.empty()) {
        for (auto head : g_extraOpts) {
            evhttp_add_header(request->output_headers, head.first.c_str(), head.second.c_str());
        }
    }

    evbuffer_add_printf(buf, "%s", content.c_str());
    evhttp_send_reply(request, message.status, "OK", buf);
    evbuffer_free(buf);
    Message("[%d]\n%s", message.status, content.c_str());
}

const char* GetMethodName(int method)
{
    const char* name = "null";
    switch (method) {
    case EVHTTP_REQ_GET: name = "GET"; break;
    case EVHTTP_REQ_POST: name = "POST"; break;
    case EVHTTP_REQ_HEAD: name = "HEAD"; break;
    case EVHTTP_REQ_PUT: name = "PUT"; break;
    case EVHTTP_REQ_DELETE: name = "DELETE"; break;
    case EVHTTP_REQ_OPTIONS: name = "OPTIONS"; break;
    case EVHTTP_REQ_TRACE: name = "TRACE"; break;
    case EVHTTP_REQ_CONNECT: name = "CONNECT"; break;
    case EVHTTP_REQ_PATCH: name = "PATCH"; break;
    default: name = "unknown"; break;
    }
    return name;
}

void Release(event_base* base, evhttp_connection* evcon)
{
    if (evcon != nullptr) {
        evhttp_connection_free(evcon);
    }
    if (base != nullptr) {
        event_base_free(base);
    }
}

void SetExtraOption(const std::string& key, const std::string& value)
{
    g_extraOpts[key] = value;
}

void RegistCallback(const std::string& name, evhttp_cmd_type method, DEALRES_CALLBACK hook)
{
    g_dealhooks[name].method = method;
    g_dealhooks[name].callback = hook;
}

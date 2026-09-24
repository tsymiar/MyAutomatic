#include "WebevApi.h"

#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/queue.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <unordered_map>
#include <chrono>
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

vector<string> g_headList = {
    "server",
    "flag",
    "username",
    "token"
};

namespace {
    const int g_wait100ms = 100000;
    const char* HTTPD_SIGNATURE = "webserver/1.0";
    static unordered_map<void*, string> g_msgRecv = {};
    static unordered_map<string, string> g_extraOpts = {};
    static unordered_map<string, DealHooks> g_dealHooks = {};
    static string g_frontendRoot = { };      // frontend static root; empty means frontend disabled
}

void Response(struct evhttp_request* request, HookDetail detail = {});
void Release(event_base* base, evhttp_connection* evcon = nullptr);

void NullFunc(...)
{
    Message("calling null func");
}

void ElegantlyBreak(void* arg)
{
    if (arg != nullptr) {
        struct event_base* ev = reinterpret_cast<struct event_base*>(arg);
        if (!event_base_got_exit(ev))
            event_base_loopexit(ev, nullptr);
    }
}

DEALRES_CALLBACK GetResHook(const string& uri, evhttp_cmd_type cmd)
{
    auto it = g_dealHooks.find(uri);
    if (it != g_dealHooks.end() && it->second.method == cmd) {
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
void RemoteConnectionCloseCallback(struct evhttp_connection* /*connection*/, void* arg)
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
    g_msgRecv[base] = string(data);
    DEALRES_CALLBACK func = g_dealHooks["handleResponse"].callback;
    if (func != nullptr) {
        HookDetail detail;
        detail.payload = data;
        func(detail, size);
    }
    fwrite("\n", 1, 1, stdout);
}

// ---------- frontend static files (default: git branch gh-pages) ----------
const char* MimeTypeOf(const string& path)
{
    static const struct { const char* ext; const char* type; } table[] = {
        { ".html", "text/html; charset=utf-8" },
        { ".htm", "text/html; charset=utf-8" },
        { ".css", "text/css; charset=utf-8" },
        { ".js", "application/javascript; charset=utf-8" },
        { ".mjs", "application/javascript; charset=utf-8" },
        { ".json", "application/json; charset=utf-8" },
        { ".map", "application/json; charset=utf-8" },
        { ".txt", "text/plain; charset=utf-8" },
        { ".xml", "application/xml; charset=utf-8" },
        { ".svg", "image/svg+xml" },
        { ".png", "image/png" },
        { ".jpg", "image/jpeg" },
        { ".jpeg", "image/jpeg" },
        { ".gif", "image/gif" },
        { ".webp", "image/webp" },
        { ".bmp", "image/bmp" },
        { ".ico", "image/x-icon" },
        { ".woff", "font/woff" },
        { ".woff2", "font/woff2" },
        { ".ttf", "font/ttf" },
        { ".otf", "font/otf" },
        { ".eot", "application/vnd.ms-fontobject" },
        { ".mp3", "audio/mpeg" },
        { ".mp4", "video/mp4" },
        { ".wasm", "application/wasm" },
        { ".pdf", "application/pdf" },
    };
    size_t dot = path.find_last_of('.');
    if (dot == string::npos) {
        return "application/octet-stream";
    }
    string ext = path.substr(dot);
    for (size_t i = 0; i < ext.size(); i++) {
        ext[i] = static_cast<char>(tolower(static_cast<unsigned char>(ext[i])));
    }
    for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); i++) {
        if (ext == table[i].ext) {
            return table[i].type;
        }
    }
    return "application/octet-stream";
}

// only accept plain tokens for branch/dir, they are used to build shell commands
bool IsSafeToken(const string& in)
{
    if (in.empty() || in.size() > 256) {
        return false;
    }
    for (size_t i = 0; i < in.size(); i++) {
        char c = in[i];
        if (!(isalnum(static_cast<unsigned char>(c)) || c == '_' || c == '-' || c == '.' || c == '/')) {
            return false;
        }
    }
    return true;
}

// map an url to a real file under the frontend root; empty result means "not served"
string ResolveFrontendFile(const string& url)
{
    if (g_frontendRoot.empty()) {
        return "";
    }
    string path = url;
    size_t stop = path.find_first_of("?#");
    if (stop != string::npos) {
        path = path.substr(0, stop);
    }
    while (!path.empty() && path[0] == '/') {
        path.erase(path.begin());
    }
    if (path.empty()) {
        path = "index.html";                    // "/" serves the landing page
    }
    if (path.find("..") != string::npos || path.find('\\') != string::npos) {
        return "";                              // reject path traversal
    }
    string full = g_frontendRoot;
    if (full[full.size() - 1] != '/') {
        full += "/";
    }
    full += path;
    if (full[full.size() - 1] == '/') {
        full += "index.html";                   // directory request serves its index
    }
    struct stat st = { };
    if (stat(full.c_str(), &st) != 0 || !S_ISREG(st.st_mode)) {
        return "";
    }
    return full;
}

void SendFrontend(struct evhttp_request* req_ptr, const string& content, int status, const char* type)
{
    struct evbuffer* buf = evbuffer_new();
    if (buf == nullptr) {
        Error("create response buffer for %zu bytes failed!", content.size());
        return;
    }
    evbuffer_add(buf, content.data(), content.size());
    struct evkeyvalq* heads = req_ptr->output_headers;
    evhttp_add_header(heads, "Server", HTTPD_SIGNATURE);
    evhttp_add_header(heads, "Content-Type", type);
    evhttp_add_header(heads, "Access-Control-Allow-Origin", "*");
    evhttp_add_header(heads, "Cache-Control", "no-cache");
    evhttp_send_reply(req_ptr, status, "OK", buf);
    evbuffer_free(buf);
}

// serve the frontend; return false when the frontend is disabled (keep legacy behavior)
bool ServeFrontend(struct evhttp_request* req_ptr, const string& url)
{
    if (g_frontendRoot.empty()) {
        return false;
    }
    string full = ResolveFrontendFile(url);
    if (full.empty()) {
        Warning("[404] frontend file not found: %s", url.c_str());
        SendFrontend(req_ptr, "404 Not Found\n", HTTP_NOTFOUND, "text/plain; charset=utf-8");
        return true;
    }
    string body = getFileAsCstring(full);
    if (body.empty()) {
        Error("[500] read frontend file failed: %s", full.c_str());
        SendFrontend(req_ptr, "500 Read File Failed\n", HTTP_INTERNAL, "text/plain; charset=utf-8");
        return true;
    }
    SendFrontend(req_ptr, body, HTTP_OK, MimeTypeOf(full));
    Message("[200] frontend: %s (%zu bytes)", full.c_str(), body.size());
    return true;
}

// registered APIs (/log branch, hooks) and OPTIONS preflight keep the legacy path
bool IsDynamicUri(const vector<string>& list, evhttp_cmd_type method)
{
    if (method == EVHTTP_REQ_OPTIONS) {
        return true;
    }
    if (!list.empty() && list[0] == "log") {
        return true;
    }
    if (list.size() > 1) {
        auto it = g_dealHooks.find(list[1]);
        if (it != g_dealHooks.end() && it->second.method == method) {
            return true;
        }
    }
    return false;
}

void SetFrontendRoot(const string& dir)
{
    g_frontendRoot = dir;
}

// locate the git repository root; git archive refuses to run when the current
// working directory itself is untracked (e.g. started from a build/ignored dir)
string DetectRepoRoot()
{
    const char* env = getenv("WEBEV_REPO");
    if (env != nullptr && env[0] != '\0') {
        return string(env);
    }
    FILE* pipe = popen("git rev-parse --show-toplevel 2>/dev/null", "r");
    if (pipe == nullptr) {
        return "";
    }
    char buf[1024] = { 0 };
    string root = "";
    if (fgets(buf, sizeof(buf), pipe) != nullptr) {
        root = buf;
    }
    pclose(pipe);
    while (!root.empty() && (root[root.size() - 1] == '\n' || root[root.size() - 1] == '\r')) {
        root.erase(root.size() - 1);
    }
    return root;
}

// export a git branch as the frontend root; every git command runs inside the repo root
int FetchFrontendFromGit(const string& branch, const string& workdir)
{
    if (!IsSafeToken(branch) || !IsSafeToken(workdir)) {
        Error("unsafe branch/workdir: '%s', '%s'!", branch.c_str(), workdir.c_str());
        return -1;
    }
    const string root = DetectRepoRoot();
    if (root.empty() || root.find('\'') != string::npos) {
        Error("git repository not found, set WEBEV_REPO=/path/to/repo!");
        return -1;
    }
    const string dir = workdir.empty() ? string("./webev-frontend") : workdir;
    if (system(("rm -rf '" + dir + "' && mkdir -p '" + dir + "'").c_str()) != 0) {
        Error("prepare frontend dir '%s' failed!", dir.c_str());
        return -1;
    }
    // keep the tarball path absolute: "git -C <root>" resolves -o against the repo root
    string tarfile = dir;
    if (tarfile[0] != '/') {
        char cwd[1024] = { 0 };
        if (getcwd(cwd, sizeof(cwd)) == nullptr) {
            Error("getcwd failed!");
            return -1;
        }
        tarfile = string(cwd) + "/" + tarfile;
    }
    tarfile += "/.frontend.tar";

    // "origin/gh-pages"/"refs/..." is used as-is, a plain name tries local then origin/
    vector<string> refs;
    refs.push_back(branch);
    if (branch.find('/') == string::npos) {
        refs.push_back("origin/" + branch);
    }
    const string git = "git -C '" + root + "' ";
    for (size_t i = 0; i < refs.size(); i++) {
        const string& ref = refs[i];
        // verify the ref first, otherwise an empty pipe would count as success
        if (system((git + "rev-parse --verify --quiet " + ref + " >/dev/null").c_str()) != 0) {
            Warning("git ref '%s' not found, try the next one", ref.c_str());
            continue;
        }
        if (system((git + "archive --format=tar -o '" + tarfile + "' " + ref).c_str()) != 0) {
            Warning("git archive '%s' failed, try the next one", ref.c_str());
            continue;
        }
        if (system(("tar -xf '" + tarfile + "' -C '" + dir + "'").c_str()) != 0) {
            Warning("extract frontend from '%s' failed, try the next one", ref.c_str());
            continue;
        }
        remove(tarfile.c_str());
        SetFrontendRoot(dir);
        Message("frontend ready: repo '%s', branch '%s' -> '%s'", root.c_str(), ref.c_str(), dir.c_str());
        return 0;
    }
    Error("no usable git branch for the frontend ('%s') in repo '%s'!", branch.c_str(), root.c_str());
    return -2;
}

// WEBEV_FRONTEND="branch[:dir]" selects the source, "0" disables the frontend
void InitFrontendFromEnv()
{
    const char* env = getenv("WEBEV_FRONTEND");
    if (env != nullptr && string(env) == "0") {
        Message("frontend disabled by env %s", env);
        return;
    }
    string branch = "gh-pages";
    string dir = "./webev-frontend";
    if (env != nullptr && env[0] != '\0') {
        string opt(env);
        size_t split = opt.find(':');
        if (split == string::npos) {
            branch = opt;
        } else {
            branch = opt.substr(0, split);
            dir = opt.substr(split + 1);
        }
    }
    FetchFrontendFromGit(branch, dir);
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
    // serve frontend files first, registered APIs keep the legacy path
    if (!IsDynamicUri(list, method) && ServeFrontend(req_ptr, url)) {
        return;
    }
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
        pid_t child = fork();
        if (child == 0) {
            if (list.size() > 0 && list[0] == "log") {
                status = HTTP_OK;
                for (auto it = g_headList.begin(); it != g_headList.end(); ++it) {
                    const char* ptr = evhttp_find_header(&head, it->c_str());
                    if (ptr == nullptr) continue;
                    string val(ptr);
                    size_t len = val.size();
                    Message("request param(%zu): %s = %s", len, it->c_str(), val.c_str());
                    if (*it == "server") {
                        string server(val);
                        uint32_t server_port = 0;
                        ssize_t pos = (server.empty() ? 0 : server.find(":"));
                        string ip = server.substr(0, pos);
                        string sub_port = server.substr(pos + 1, server.size() - 1);
                        if (isNum(sub_port)) {
                            server_port = atoi(sub_port.c_str());
                        }
                        if (pos <= 0) {
                            Error("can't find ':' in '%s'!", server.c_str());
                            status = HTTP_NOTIMPLEMENTED;
                            break;
                        }
                        Message("server parse = %s:%u", ip.c_str(), server_port);
                        size_t post_len = evbuffer_get_length(req_ptr->input_buffer);
                        char* post_data = (char*)evbuffer_pullup(req_ptr->input_buffer, post_len);
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
            if (write(filedes[1], &status, sizeof(int)) < 0) {
                Error("write status to pipe failed!");
            }
            exit(0);
        } else if (child > 0) {
            pid_t pid = 0;
            do {
                pid = waitpid(child, nullptr, WNOHANG);
            } while (pid == 0);
            close(filedes[1]);
            int childStatus = 0;
            const ssize_t stLen = read(filedes[0], &childStatus, sizeof(childStatus));
            if (stLen == (ssize_t)sizeof(childStatus)) {
                status = childStatus; // 仅完整读取后才生效, 避免半包数据被当作状态
            } else {
                Error("failed to read status from filedes[0]");
            }
            HookDetail message = {};
            message.msg = "OK";
            message.method = method;
            message.status = status;
            Response(req_ptr, message);
            if (pid == child) {
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
    while (base == nullptr || g_msgRecv[base].empty()) {
        if (count > 3)
            break;
        this_thread::sleep_for(chrono::microseconds(g_wait100ms));
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
    if (path == nullptr || path[0] == '\0') {
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
    short int port = evhttp_uri_get_port(uri);
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
    if (!g_extraOpts.empty()) {
        for (auto head : g_extraOpts) {
            evhttp_add_header(output_headers, head.first.c_str(), head.second.c_str());
        }
    }

    if (detail.filename != nullptr) {
        char conBuf[256] = { 0 };
        snprintf(conBuf, 256, "http://%s", host);
        evhttp_add_header(output_headers, "Origin", conBuf);
        snprintf(conBuf, 48, "application/x-www-form-urlencoded;charset=utf-8");
        Message("setting '%s' form-data", detail.filename);

        evbuffer* output_buffer = evhttp_request_get_output_buffer(request);
        const char boundary[] = "------WebKitFormBoundaryAu886z32WLCM1Fl0\r\n";
        int bndLen = (int)sizeof(boundary) - 1;
        evbuffer_add(output_buffer, boundary, (size_t)bndLen);

        char bndBuf[256];
        snprintf(bndBuf, sizeof(bndBuf), "Content-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\n", detail.filename);
        size_t bndBufLen = strnlen(bndBuf, sizeof(bndBuf));
        bndLen += (int)bndBufLen;
        evbuffer_add(output_buffer, bndBuf, bndBufLen);

        snprintf(bndBuf, sizeof(bndBuf), "Content-Type: application/octet-stream\r\n\r\n");
        bndBufLen = strnlen(bndBuf, sizeof(bndBuf));
        bndLen += (int)bndBufLen;
        evbuffer_add(output_buffer, bndBuf, bndBufLen);

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

        snprintf(bndBuf, sizeof(bndBuf), "\r\n------WebKitFormBoundaryAu886z32WLCM1Fl0--\r\n");
        bndBufLen = strnlen(bndBuf, sizeof(bndBuf)); // 有界取值, 防止未 '\0' 结尾时越界读
        bndLen += (int)bndBufLen;
        evbuffer_add(output_buffer, bndBuf, bndBufLen);

        fclose(fd);
        evutil_snprintf(buf, sizeof(buf) - 1, "%lu", (unsigned long)bts + bndLen);
        evhttp_add_header(output_headers, "Content-Length", buf);
        snprintf(conBuf, 256, "multipart/form-data;boundary=%s", boundary);
        evhttp_add_header(output_headers, "Content-Type", conBuf);
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
    InitFrontendFromEnv();
    Message("Http server start over [%d] OK!", port);

    event_base_dispatch(base);
    evhttp_free(http);
    return 0;
}

int RequestClient(const char* url, HookDetail& detail, DEALRES_CALLBACK hook)
{
    int stat = -1;
    thread client([&stat, &detail](const char* target) {
        detail.url = target;
        stat = HttpClient(detail);
        }, url);
    if (hook != nullptr) {
        WaitMsgTask(detail.base);
        if (client.joinable())
            client.join();
    } else {
        g_dealHooks["handleResponse"].callback = hook;
        client.detach();
        this_thread::sleep_for(chrono::microseconds(g_wait100ms));
    }
    for (auto msg : g_msgRecv) {
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
        Error("client request invalid!");
        return;
    }
    evhttp_add_header(request->output_headers, "Server", HTTPD_SIGNATURE);
    evhttp_add_header(request->output_headers, "Access-Control-Allow-Origin", "*");
    evhttp_add_header(request->output_headers, "Access-Control-Allow-Method", "POST, GET, OPTIONS");
    evhttp_add_header(request->output_headers, "Access-Control-Allow-Headers", "X-PINGOTHER, Content-Type");
    evhttp_add_header(request->output_headers, "Access-Control-Max-Age", "1728000");
    evhttp_add_header(request->output_headers, "X-Xss-Protection", "1; mode=block");
    evhttp_add_header(request->output_headers, "Connection", "close");
    if (!g_extraOpts.empty()) {
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

void SetExtraOption(const string& key, const string& value)
{
    g_extraOpts[key] = value;
}

void RegisterCallback(const string& name, evhttp_cmd_type method, DEALRES_CALLBACK hook)
{
    g_dealHooks[name].method = method;
    g_dealHooks[name].callback = hook;
}

void SetHeadsList(const vector<string>& heads)
{
    g_headList = heads;
}

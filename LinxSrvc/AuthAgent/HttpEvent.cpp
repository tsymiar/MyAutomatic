#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>
#include <string>

#include "event2/http.h"
#include "event2/http_struct.h"
#include "event2/event.h"
#include "event2/buffer.h"
#include "event2/dns.h"
#include "event2/thread.h"
#include "event2/keyvalq_struct.h" 
#include "event2/buffer_compat.h"

#include "Logging.h"

int StartClient(const char* url, const char* filename = NULL);

void RemoteReadCallback(struct evhttp_request* remote_rsp, void* arg)
{
    event_base_loopexit((struct event_base*)arg, NULL);
}

int ReadHeaderDoneCallback(struct evhttp_request* remote_rsp, void* arg)
{
    Message("< HTTP/1.1 %d %s", evhttp_request_get_response_code(remote_rsp), evhttp_request_get_response_code_line(remote_rsp));
    struct evkeyvalq* headers = evhttp_request_get_input_headers(remote_rsp);
    struct evkeyval* header;
    TAILQ_FOREACH(header, headers, next) {
        Message("< %s: %s", header->key, header->value);
    }
    Message("< ");
    return 0;
}

void ReadChunkCallback(struct evhttp_request* remote_rsp, void* arg)
{
    char buf[4096];
    struct evbuffer* evbuf = evhttp_request_get_input_buffer(remote_rsp);
    int n = 0;
    while ((n = evbuffer_remove(evbuf, buf, 4096)) > 0) {
        fwrite(buf, n, 1, stdout);
    }
}

void RemoteRequestErrorCallback(enum evhttp_request_error error, void* arg)
{
    Error("request failed!");
    event_base_loopexit((struct event_base*)arg, NULL);
}

void RemoteConnectionCloseCallback(struct evhttp_connection* connection, void* arg)
{
    Error("remote connection closed!");
    event_base_loopexit((struct event_base*)arg, NULL);
}

int StartClient(const char* url, const char* filename)
{
    const char* user_agent = "Mozilla/5.0 (Macintosh;"
        " Intel Mac OS X 10_15_7) AppleWebKit/605.1.15 (KHTML, like Gecko) Version/15.0 Safari/605.1.15";
    const char* path = NULL;
    const char* host = NULL;
    int port = 0;
    char conbuf[256] = { 0 };

    struct evhttp_uri* uri = NULL;
    struct evhttp_request* request = NULL;
    struct evhttp_connection* connect = NULL;
    struct evkeyvalq* output_headers = NULL;

    struct event_base* base = event_base_new();
    if (!base) {
        Error("create event base failed!");
        return -1;
    }

    uri = evhttp_uri_parse(url);
    if (!uri) {
        Error("parse url failed!");
        goto error;
        return -2;
    }
    path = evhttp_uri_get_path(uri);
    if (path == NULL || strlen(path) == 0) {
        url = "/";
    }

    request = evhttp_request_new(RemoteReadCallback, base);
    // Fire off the request
    if (request == NULL) {
        Error("evhttp_request_new() failed!");
        goto error;
        return -3;
    }

    host = evhttp_uri_get_host(uri);
    if (!host) {
        Error("parse host failed!");
        goto error;
        return -4;
    }
    port = evhttp_uri_get_port(uri);
    if (port < 0) port = 80;

    Message("url: %s host: %s port: %d path: %s", url, host, port, path);

    connect = evhttp_connection_base_new(base, NULL, host, port);
    if (!connect) {
        Error("create evhttp connection failed!");
        goto error;
        return -5;
    }
    evhttp_connection_set_closecb(connect, RemoteConnectionCloseCallback, base);
    evhttp_connection_set_timeout(request->evcon, 600);
    Message("evhttp_connection_get_bufferevent() ok");

    evhttp_request_set_header_cb(request, ReadHeaderDoneCallback);
    evhttp_request_set_chunked_cb(request, ReadChunkCallback);
    evhttp_request_set_error_cb(request, RemoteRequestErrorCallback);

    output_headers = evhttp_request_get_output_headers(request);
    evhttp_add_header(output_headers, "Host", host);
    evhttp_add_header(output_headers, "Connection", "keep-alive");
    evhttp_add_header(output_headers, "User-Agent", user_agent);
    evhttp_add_header(output_headers, "Accept", "*/*");
    evhttp_add_header(output_headers, "Accept-Encoding", "gzip,deflate,br");
    evhttp_add_header(output_headers, "Accept-Language", "zh-CN,zh;q=0.8");
    evhttp_add_header(output_headers, "Cache-Control", "max-age=0");

    sprintf(conbuf, "http://%s", host);
    evhttp_add_header(output_headers, "Origin", conbuf);
    sprintf(conbuf, "application/x-www-form-urlencoded;charset=utf-8");
    if (filename != NULL) {
        Message("setting '%s' form-data", filename);
        evbuffer* output_buffer = evhttp_request_get_output_buffer(request);
        const char* boundary = "------WebKitFormBoundaryAu886z32WLCM1Fl0\r\n";
        int bndlen = strlen(boundary);
        evbuffer_add(output_buffer, boundary, bndlen);

        char bndbuf[256];
        sprintf(bndbuf, "Content-Disposition: form-data; name=\"file\"; filename=\"%s\"\r\n", filename);
        bndlen += strlen(bndbuf);
        evbuffer_add(output_buffer, bndbuf, strlen(bndbuf));

        sprintf(bndbuf, "Content-Type: application/octet-stream\r\n\r\n");
        bndlen += strlen(bndbuf);
        evbuffer_add(output_buffer, bndbuf, strlen(bndbuf));

        FILE* fd = fopen(filename, "rb");
        char buf[1024];
        size_t s;
        size_t bts = 0;

        if (!fd) {
            Error("Open file '%s' error!", filename);
            goto error;
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
    }
    evhttp_add_header(output_headers, "Content-Type", conbuf);

    evhttp_make_request(connect, request, EVHTTP_REQ_POST, url);
    event_base_dispatch(base);
    goto error;

error:
    {
        if (!base) {
            event_base_free(base);
        }
        if (!request) {
            evhttp_request_free(request);
        }
        if (!connect) {
            evhttp_connection_free(connect);
        }
    }

    return 0;
}

void GenericHandler(struct evhttp_request* req, void* arg)
{
    const char* req_uri = evhttp_request_get_uri(req);
    char* dec_uri = evhttp_decode_uri(req_uri);
    Message("request url: %s", dec_uri);
    struct evkeyvalq head;
    evhttp_parse_query(dec_uri, &head);
    const char* param = evhttp_find_header(&head, "server");
    Message("request param: server = %s", param);
    param = (param == NULL ? "" : param);
    std::string ip;
    char* post_data;
    std::string server(param);
    size_t pos = (server.empty() ? 0 : server.find(":"));
    if (pos <= 0) {
        Error("can't find ':' in '%s'!", server.c_str());
        goto success;
        return;
    }
    ip = server.substr(0, pos);
    Message("request ip = %s", ip.c_str());
    post_data = (char*)EVBUFFER_DATA(req->input_buffer);
    Message("request post_data = %s", post_data);
    free(dec_uri);
    goto success;

success:
    {
        struct evbuffer* buf = evbuffer_new();
        if (!buf) {
            Message("failed to create response buffer!");
            return;
        }
        const char* reply = "{\n\tstatus:true,\n\terrno:200\n}";
        evbuffer_add_printf(buf, "%s", reply);
        evhttp_send_reply(req, HTTP_OK, "OK", buf);
        evbuffer_free(buf);
    }
}

int StartServer(short port)
{
    const char* addr = "127.0.0.1";

    struct event_base* base = event_base_new();
    if (!base) {
        Error("create event base failed!");
        return -1;
    }

    struct evhttp* http = evhttp_new(base);
    if (!http) {
        Error("http server malloc failed!");
        return -1;
    }

    int ret = evhttp_bind_socket(http, addr, port);
    if (ret != 0) {
        Error("http bind socket failed: %d!", port);
        return -1;
    }

    evhttp_set_gencb(http, GenericHandler, NULL);
    Message("http server start OK!");

    event_base_dispatch(base);
    evhttp_free(http);
    return 0;
}

#pragma once
#include <string>
#include <vector>
#include "event2/http.h"

struct HookDetail {
    int status = HTTP_NOTIMPLEMENTED;
    evhttp_cmd_type method = EVHTTP_REQ_GET;
    struct event_base* base = nullptr;
    const char* payload = nullptr;
    const char* filename = nullptr;
    std::string msg = {};
    std::string url = {};
};

typedef struct HookDetail(*PARSREQ_CALLBACK)(const std::vector<std::string>&, evhttp_cmd_type, char*);
typedef void(*PACKRSP_CALLBACK)(HookDetail&);

struct SrvCallbacks {
    PARSREQ_CALLBACK ParsReq;
    PACKRSP_CALLBACK PackRsp;
};

typedef void(*DEALRES_CALLBACK)(const HookDetail&, size_t);

struct DealHooks {
    evhttp_cmd_type method;
    DEALRES_CALLBACK callback;
};

int StartServer(short, struct SrvCallbacks* = nullptr);
int RequestClient(const char*, HookDetail&, DEALRES_CALLBACK = nullptr);
void RegistCallback(std::string, DEALRES_CALLBACK);
void SetExtraOption(std::string, std::string);

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

typedef void(*DEALRES_CALLBACK)(const HookDetail&);
typedef struct HookDetail(*PARSREQ_CALLBACK)(const std::vector<std::string>&, evhttp_cmd_type, char*);
typedef void(*PACKRSP_CALLBACK)(HookDetail&);

struct SrvCallbacks {
    PARSREQ_CALLBACK ParsReq;
    PACKRSP_CALLBACK PackRsp;
};

int StartServer(short, struct SrvCallbacks* = nullptr);
int ClientRequest(const char*, HookDetail&);
void RegisterCallbacks(DEALRES_CALLBACK);
void SetExtraOption(std::string, std::string);

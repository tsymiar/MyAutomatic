#pragma once
#include "stdafx.h"
#include <string>

static CString httpGetReq(std::string& sRequestUrl, const std::string& sPort = "443", const std::string& proxyUsername = "User-001", const std::string& proxyPassword = "111111");

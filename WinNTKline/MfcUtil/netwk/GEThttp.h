#pragma once
#include "stdafx.h"
#include <wininet.h>
#include <string>

CString HttpGet(std::string& sRequest, std::string sPort = "443");
#pragma once
#include "stdafx.h"
#include <string>

CString HttpGet(std::string& sRequest, const std::string& sPort = "443");

#pragma once
#include "stdafx.h"
#include <wininet.h>
#include <string>

CString HttpGet(string& sRequest, string sPort = "443");
#!/bin/bash
soapcpp2 -C rpcapi.h
soapcpp2 -i rpcapi.h
soapcpp2 -S rpcapi.h
wsdl2h -o myweb.h myweb.wsdl
# Windows 侧（MFCKline/TestUtils 的 vcxproj）编译的 fix/soapClient_fix.cpp
# 由下面的脚本从 soapClient.cpp 派生，不再手工维护副本
# sed -i '/soap_call_/,+5 {H; d}; $ {p; x; s/ ^ \n //}' soapClient.cpp
python3 "$(dirname "$0")/gen_client_fix.py"

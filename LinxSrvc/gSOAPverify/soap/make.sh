soapcpp2 -C rpcapi.h
soapcpp2 -i rpcapi.h
soapcpp2 -S rpcapi.h
wsdl2h -o myweb.h myweb.wsdl
#sed -i '/soap_call_/,+5 {H; d}; $ {p; x; s/ ^ \n //}' soapClient.cpp

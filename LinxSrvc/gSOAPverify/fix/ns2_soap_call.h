#include <vector>
#include "../soap/soapStub.h"
#define SOAP_NAMESPACE_OF_ns2	"urn:myweb"

typedef std::string wsdl__xsd_string;
typedef std::string xsd__xsd_int;
typedef std::string xsd__xsd_string;

class ns2__result {
public:
  /// Element "flag" of type xs:xsd-int.
  xsd__xsd_int                         flag{ 1 };	///< Required element.
  /// Element "idx" of type xs:xsd-int.
  xsd__xsd_int                         idx{ 1 };	///< Required element.
  /// Element "age" of type xs:xsd-int.
  xsd__xsd_int                         age{ 1 };	///< Required element.
  /// Element "tell" of type xs:xsd-string.
  xsd__xsd_string* tell = NULL;	///< Optional element.
  /// Element "email" of type xs:xsd-string.
  xsd__xsd_string* email = 0;	///< Optional element.
  /// Pointer to soap context that manages this instance.
  struct soap* soap;
};

int ns2__trans(
  wsdl__xsd_string  msg,	///< Input parameter, :unqualified name as per RPC encoding
  wsdl__xsd_string& rtn	///< Output parameter, :unqualified name as per RPC encoding
);

int ns2__get_server_status(
  wsdl__xsd_string  req,	///< Input parameter, :unqualified name as per RPC encoding
  wsdl__xsd_string& rsp	///< Output parameter, :unqualified name as per RPC encoding
);

/// Operation response struct "ns2__login_by_keyResponse" of operation "ns2__login_by_key".
struct ns2__login_by_keyResponse {
  ns2__result* rslt;	///< Output parameter, :unqualified name as per RPC encoding
};

//gsoap ns2  service method-protocol:	login_by_key SOAP
//gsoap ns2  service method-style:	login_by_key rpc
//gsoap ns2  service method-encoding:	login_by_key http://schemas.xmlsoap.org/soap/encoding/
//gsoap ns2  service method-action:	login_by_key ""
//gsoap ns2  service method-output-action:	login_by_key Response
int ns2__login_by_key(
  wsdl__xsd_string usr,	///< Input parameter, :unqualified name as per RPC encoding
  wsdl__xsd_string psw,	///< Input parameter, :unqualified name as per RPC encoding
  struct ns2__login_by_keyResponse&	///< Output response struct parameter
);

SOAP_FMAC5 int SOAP_FMAC6 soap_call_api__trans(struct soap* soap, const char* soap_endpoint, const char* soap_action, char* msg, char** rtn);
SOAP_FMAC5 int SOAP_FMAC6 soap_call_api__get_server_status(struct soap* soap, const char* soap_endpoint, const char* soap_action, char* req, char*& rsp);
SOAP_FMAC5 int SOAP_FMAC6 soap_call_api__login_by_key(struct soap* soap, const char* soap_endpoint, const char* soap_action, char* usr, char* psw, struct api__ArrayOfEmp2& stat);

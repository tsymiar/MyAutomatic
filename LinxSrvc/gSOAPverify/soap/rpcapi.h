//gsoap api service name: myweb
//gsoap api service style: rpc
//gsoap api service executable: myweb.cgi 
//gsoap api service encoding: encoded
//gsoap api service namespace: http://localhost/myweb.wsdl
//gsoap api service location: http://localhost/myweb.cgi
//gsoap api schema  namespace: urn:myweb

typedef		char*	xsd_string;
typedef		long	xsd_int;

struct api__result
{
	xsd_int flag;	
	xsd_int idx;
	xsd_int age;
	xsd_string tell;
	xsd_string email;
};

struct api__ArrayOfEmp2
{
	struct api__result rslt;
};

int api__trans(xsd_string msg, xsd_string rtn[]);
int api__get_server_status(xsd_string req, xsd_string& rsp);
int api__login_by_key(xsd_string usr, xsd_string psw, struct api__ArrayOfEmp2& stat);

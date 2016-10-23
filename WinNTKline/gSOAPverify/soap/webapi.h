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
	xsd_int id;
	xsd_int age;
	xsd_string user;
	xsd_string email;
};

int api__encrypt(char* input, char** output);
int api__login_by_key(xsd_string usr, xsd_string psw, struct api__result &flag);
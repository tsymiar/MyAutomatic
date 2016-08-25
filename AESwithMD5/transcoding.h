#ifndef TRANSCODING_H
#define TRANSCODING_H
#include <stddef.h>
#include <string.h>
#ifdef _LIBICONV_H
	#include<iconv.h>
	#ifdef _MSC_VER
	#pragma comment(lib,"iconv.lib")
	#endif
#endif
#ifdef __cplusplus
extern "C"
{
#endif
	int str_to_hex(char *string, char *cbuf);
	void hex_to_str(unsigned char *dest, char *pbSrc);
#ifdef _LIBICONV_H
	int conv_charset(const char* dest,const char* pbSrc,const char *input,size_t ilen,char* output,size_t olen);
#endif
#ifdef __cplusplus
}
#endif
#endif
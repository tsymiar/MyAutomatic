#ifndef MARKDN_H
#define MARKDN_H
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cassert>
#ifdef __linux
#include <unistd.h>
#endif
#include <openssl/idea.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include "MD5.h"

#define  _RSAref_MAX_LEN 128

typedef struct
{
	unsigned int bits;
	unsigned char modulus[_RSAref_MAX_LEN];
	unsigned char exponent[_RSAref_MAX_LEN];
}RSArefPubKey_st;   

struct KEYINFO{
	unsigned char Index;
	unsigned char KeyNum;
	unsigned char AsBankNO[13];
	unsigned char NewEncKey[16];
	unsigned char NewKeyStartDate[8];
	unsigned char CurrEncKey[16];
	unsigned char CurrKeyEndDate[8];	
	unsigned char DAC[ 8 ];
};

#ifdef __cplusplus
extern "C"
{
#endif

void test_verify();
char* extra_8bt(char* prim, char* bt, int h);
unsigned int get_CRC32(char* InStr,unsigned int len);
unsigned short get_CRC16(char* InStr,unsigned int len);
unsigned char* get_hash1(unsigned char* ha,unsigned  char* sh, unsigned char* sa);

#ifdef __cplusplus
}
#endif

#endif

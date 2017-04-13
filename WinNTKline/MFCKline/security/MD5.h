#pragma once
#ifndef MD5_H
#define MD5_H

#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "encoding.h"

#ifdef __cplusplus
extern "C"
{
#endif

	void md5_str(char *input, char *output);
	void md5_file(FILE *fp, char out[]);
	char* get_Hash(char *md5, int len, char *dst);
	void test_f_md5();
	void test_s_md5();

#ifdef __cplusplus
}
#endif
#endif

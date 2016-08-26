#ifndef MD5_H
#define MD5_H

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>

#include "encoding.h"

#ifdef __cplusplus
extern "C"
{
#endif

void md5_str(char *input, char *output);
void md5_file(FILE *fp, char out[]);
void test_f_md5();
void test_s_md5();

#ifdef __cplusplus
}
#endif
#endif
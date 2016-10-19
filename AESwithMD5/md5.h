#pragma once
#ifndef MD5_H
#define MD5_H

#include<stdio.h>
#include<stdlib.h>
#include<stdint.h>
#include<string.h>

#include "transcoding.h"

#ifdef __cplusplus
extern "C"
{
#endif

void md5_str(char *input, char *output);
void md5_file(FILE *fp, char out[]);
void test_f_md5();
void test_s_md5();
char* get_Hash(char *md5, char *dst);
unsigned short GetCrc16(char* InStr,unsigned int len);
unsigned int GetCrc32(char* InStr,unsigned int len);

#ifdef __cplusplus
}
#endif
#endif
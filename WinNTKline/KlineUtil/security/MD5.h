#ifndef MD5_H
#define MD5_H

#include <stdio.h>

#ifdef __cplusplus
extern "C"
{
#endif

    void open_md5_file(FILE* fp, char out[]);
    void md5_to_hex(char* input, char* output);
    char* get_Hash(char* md5, int len, char* dst);
    void hex_to_ascii(unsigned char* hex, char* ascii);
    int ascii_to_hex(char* ascii, char* hex);
    void test_f_md5();
    void test_s_md5();

#ifdef __cplusplus
}
#endif
#endif // MD5_H

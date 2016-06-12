/*
 * Advanced Encryption Standard
 * @author Dani Huertas
 * @email huertas.dani@gmail.com
 * @last edit tsymiar
 * Based on the document FIPS PUB 197
 */
#ifndef AES_H
#define AES_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C"
{
#endif

	//加密
	void en_cipher(uint8_t *in, uint8_t *out, uint8_t *w);
	//解密
	void inv_cipher(uint8_t *in, uint8_t *out, uint8_t *w);
	//key长度
	int key_len(uint8_t key[]);
	//扩展密钥
	void key_exp(uint8_t *key, uint8_t *w);
	//测试
	void test_aes();

#ifdef __cplusplus
}
#endif
#endif
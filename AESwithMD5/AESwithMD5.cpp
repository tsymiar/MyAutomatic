#include <stdio.h>
#include "aes.h"
#include "md5.h"
#include "transcoding.h"
int main(int argc, char *argv[]) {
	int i,len;
	char in[256];
	char out[64];
	char key[16];
	uint8_t *ek = 0;
	uint8_t cipher[32]; // 128
	uint8_t encrypt[64];
	while (1)
	{
		printf("请输入要计算MD5值的字符串:\n");
	#ifdef _MSC_VER
		gets_s(in);
	#else
		gets(in);
	#endif
		md5_str(in, key);
		//printf("MD5:");
		//for (i = 0; i<16; i++) //16位无符号整数
		//	printf("%02x", (unsigned char)key[i]);
		//printf("\n");
		hex_to_str((unsigned char*)key, out);
		printf("以源字符串值为密钥\n");
		len=key_len((uint8_t *)in);
		ek = (uint8_t*)malloc(len);
		key_exp((uint8_t*)in, ek);
		printf("Key:%s\n", ek);
		hex_to_str((unsigned char*)key, out);
		printf("输入:%s\n", out);
		en_cipher((uint8_t*)key, cipher, ek);
		hex_to_str(cipher, out);
		printf("密文:%s\n", out);
		inv_cipher(cipher, encrypt, ek);
		hex_to_str(encrypt, out);
		printf("解密:%s\n", out);
		//getchar();
		printf("\n");
	}
	//exit(0);
}

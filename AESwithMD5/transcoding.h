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
	int	 str_to_hex(char *string, char *cbuf);
	void hex_to_str(unsigned char *pbSrc, char *dest);
	void hex_to_asc(unsigned char *asc,unsigned char *hex,int len);
	void asc_to_hex(unsigned char *hex,unsigned char *asc,int len);
	void asc_to_ebcd(unsigned char *ebcd,unsigned char *asc,int len);
	void ebcd_to_asc(unsigned char *asc,unsigned char *ebcd,int len);
	void BCD2Bin(unsigned char  *BCDBuf,unsigned char  BCDLen,unsigned char  *BinBuf);
	void ASC2Bin(unsigned char  *ASCBuf,unsigned char  ASCLen,unsigned char  *BinBuf);
	void Bin2ASC(unsigned char  *BinBuf,unsigned char  BinLen,unsigned char  *ASCBuf);
	void Bin2BCD(unsigned char  *BinBuf,unsigned char  BinLen,unsigned char  *BCDBuf);
	int	Dec2Hex(int dec, unsigned char *hex, int length);
	int	Dec2BCD(int Dec, unsigned char *BCD, int length);
	void BCD2Bin(unsigned char* BCDBuf,unsigned char  BCDLen,unsigned char* BinBuf);
	void Hex2BCD(char* HEX, char* BCD);
	unsigned long Hex2Dec(const unsigned char *hex, int length);
	unsigned long  BCD2Dec(const unsigned char *BCD, int length);
#ifdef _LIBICONV_H
	int conv_charset(const char* dest,const char* pbSrc,const char *input,size_t ilen,char* output,size_t olen);
#endif
#ifdef __cplusplus
}
#endif
#endif
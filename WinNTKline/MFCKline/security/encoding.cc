#include "encoding.h"

#ifdef _LIBICONV_H
//remarks:国标码与UTF-8互转
//[IN] dest:转码格式,如“GBK”,"UTF-8";pbSrc:原码格式;input:原码字符串;ilen:原码字符串长度;
//[OUT] output:转码字符串;olen:转码字符串长度。
int conv_charset(const char* dest,const char* pbSrc,const char *input,size_t ilen,char* output,size_t olen)
	{
		int convlen=(int)olen;
		iconv_t conv=iconv_open(dest,pbSrc);
		if(conv==(iconv_t)-1)
			return -1;
		memset(output,0,olen);
		if(iconv(conv,&input,&ilen,&output,&olen))
			return -1;
		iconv_close(conv);
		return (int)(convlen-olen);
	}
#endif // _LIBICONV_H

//remarks:字符串转换为16进制数
//[IN]:string 源字符串
//[OUT]:cbuf 16进制转码字符
int str_to_hex(char *string, char *cbuf)  
{  
	int len = strlen(string);
    unsigned char high, low;  
    int idx, ii=0;  
    for (idx=0; idx<len; idx+=2)   
    {  
        high = string[idx];  
        low = string[idx+1];  
          
        if(high>='0' && high<='9')  
            high = high-'0';  
        else if(high>='A' && high<='F')  
            high = high - 'A' + 10;  
        else if(high>='a' && high<='f')  
            high = high - 'a' + 10;  
        else  
            return -1;  
          
        if(low>='0' && low<='9')  
            low = low-'0';  
        else if(low>='A' && low<='F')  
            low = low - 'A' + 10;  
        else if(low>='a' && low<='f')  
            low = low - 'a' + 10;  
        else  
            return -1;  
          
        cbuf[ii++] = high<<4 | low;  
    }  
    return 0;  
}  
/*
// C prototype : void hex_to_str(unsigned char *dest, char *pbSrc)
// parameter(s): [OUT] pbDest - 存放目标字符串
//	[IN] pbSrc - 输入16进制数的起始地址
//	（[IN] len - 16进制数的字节数
// void hex_to_str(unsigned char *dest, unsigned char *pbSrc, int len)）
// return value: 
// remarks : 将16进制数转化为ASCII码格式字符串
*/
void hex_to_str(unsigned char *dest, char *pbSrc)
{
	int i;
	char ddl,ddh;
	int len=sizeof(dest) * 4;
	if(dest==NULL)
		return;
	for (i=0; i<len; i++)
	{
		ddh = 48 + dest[i] / 16;
		ddl = 48 + dest[i] % 16;
		if (ddh > 57) ddh = ddh + 7 + 32;
		if (ddl > 57) ddl = ddl + 7 + 32;
		pbSrc[i*2] = ddh;
		pbSrc[i*2+1] = ddl;
	}
	pbSrc[len*2] = '\0';
}

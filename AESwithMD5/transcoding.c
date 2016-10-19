#include "transcoding.h"
#ifdef _LIBICONV_H
///////////////////////////////////////////////////////////////////////////////////////
//功能:	国标码与UTF-8互转
//[IN]	dest:	转码格式,如“GBK”,"UTF-8";pbSrc:原码格式;input:原码字符串;ilen:原码字符串长度;
//[OUT] output:	转码字符串;olen:转码字符串长度。
//[RET]			不为-1成功
///////////////////////////////////////////////////////////////////////////////////////
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
/////////////////////////////////////////////////////   
//功能：二进制取反   
//[IN]：const unsigned char *src  二进制数据   
//      int length                待转换的二进制数据长度   
//[OUT]：unsigned char *dst       取反后的二进制数据   
//[RET]：0    success   
//////////////////////////////////////////////////////   
int convert(unsigned char *dst, const unsigned char *src, int length)  
{  
    int i;  
    for (i = 0; i < length; i++)  
    {  
        dst[i] = src[i] ^ 0xFF;  
    }  
    return 0;  
}  
/////////////////////////////////////////////////////////   
//功能：求权   
//[IN]：int base                    进制基数   
//      int times                   权级数   
//[RET]：unsigned long              当前数据位的权   
/////////////////////////////////////////////////////////
unsigned long power(int base, int times)  
{  
    int i;  
    unsigned long rslt = 1;  
    for (i = 0; i < times; i++)  
        rslt *= base;  
    return rslt;  
}  
//[MK]:	字符串转换为16进制数
//[IN]:	string 源字符串
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
// C prototype : void hex_to_str(unsigned char *pbSrc, char *dest)
// parameter(s): [OUT] dest - 存放目标字符串
//	[IN] pbSrc - 输入16进制数的起始地址
// void hex_to_str(unsigned char *pbSrc, char *dest)）
// return value: 
// remarks : 将16进制数转化为ASCII码格式字符串
*/
void hex_to_str(unsigned char *pbSrc, char *dest)
{
	int i;
	char ddl,ddh;
	int len=sizeof(pbSrc) * 4;
	if(pbSrc==NULL)
		return;
	for (i=0; i<len; i++)
	{
		ddh = 48 + pbSrc[i] / 16;
		ddl = 48 + pbSrc[i] % 16;
		if (ddh > 57) ddh = ddh + 7 + 32;
		if (ddl > 57) ddl = ddl + 7 + 32;
		dest[i*2] = ddh;
		dest[i*2+1] = ddl;
	}
	dest[len*2] = '\0';
}

void hex_to_asc(unsigned char *asc,unsigned char *hex,int len)
{
  int i;
  int ch;
  for(i=0;i<len;i++)
  {

    ch=hex[i]>>4;
    if(ch>=0&&ch<=9)
    {
       asc[i*2]=ch+'0';
    }
    if(ch>=0xA&&ch<=0xF)
    {
       asc[i*2]=ch-0xa+'A';
    }


    ch=hex[i]&0x0F;
    if(ch>=0&&ch<=9)
    {
       asc[i*2+1]=ch+'0';
    }
    if(ch>=0xA&&ch<=0xF)
    {
       asc[i*2+1]=ch-0xa+'A';
    }
  }
}

void asc_to_hex(unsigned char *hex,unsigned char *asc,int len)
{
	int i;
	unsigned char ch;
  for(i=0;i<len;i++)
  {
    ch=asc[i];
    if(ch>='0'&&ch<='9')
    {
        ch-='0';
    }
    if(ch>='a'&&ch<='f')
    {
        ch=ch-'a';
        ch=ch+10;
    }
    if(ch>='A'&&ch<='F')
    {
        ch=ch-'A';
        ch=ch+10;
    }

    if(i%2==0)
    {
       hex[i/2]=ch<<4;
    }else
    {
       hex[i/2]=hex[i/2]|ch;
    }

  }
}

void asc_to_ebcd(unsigned char *ebcd,unsigned char *asc,int len)
{
	int i;

	for(i=0;i<len;i++)
	{
		if((asc[i]>='0') && (asc[i]<='9'))
			ebcd[i]=asc[i]-'0' + 0xF0;
		if((asc[i]>='A') && (asc[i]<='F'))
			ebcd[i]=asc[i]-'A' + 0xC1;
		if((asc[i]>='a') && (asc[i]<='f'))
			ebcd[i]=asc[i]-'a' + 0xC1;
	}

}

void ebcd_to_asc(unsigned char *asc,unsigned char *ebcd,int len)
{
	int i;

	for(i=0;i<len;i++){
		if((ebcd[i]>=0xF0) && (ebcd[i]<=0xF9))
			asc[i]=ebcd[i]-0xF0+'0';
		if((ebcd[i]>=0xC1) && (ebcd[i]<=0xC6))
			asc[i]=ebcd[i]-0xC1+'A';
	}
}

/************** 将BCD字符串展开为BIN字符串**************************************
输入参数:
	BCDBuf: BCD字符串
	BCDLen: BCD字符串的长度
输出参数:
	BINBuf: BIN字符串(即ASC码-'0')
返回值:	无
********************************************************************************/
void BCD2Bin(unsigned char  *BCDBuf,unsigned char  BCDLen,unsigned char  *BinBuf)
{
	unsigned char  i,j;
	for(i=BCDLen;i!=0;i--)
	{
		j=BCDBuf[i-1];
		BinBuf[i*2-1]=j&0x0F;
		BinBuf[i*2-2]=j>>4;
	}
}
/************** 将ASC字符串展开为BIN字符串**************************************
输入参数:
	ASCBuf: ASC字符串
	ASCLen: ASC字符串的长度
输出参数:
	BINBuf: BIN字符串(即ASC码-'0')
返回值:	无
********************************************************************************/
void ASC2Bin(unsigned char  *ASCBuf,unsigned char  ASCLen,unsigned char  *BinBuf)
{
	unsigned char  i;
	for(i=0;i<=ASCLen;i++)
	{
		BinBuf[i]=ASCBuf[i]-0x30;
	}
}
/************** 将BIN字符串转变为ASC字符串**************************************
输入参数:
	BINBuf: BIN字符串
	BINLen: BIN字符串的长度
输出参数:
	ASCBuf: ASC字符串(即BIN码+'0')
返回值:	无
********************************************************************************/
void Bin2ASC(unsigned char  *BinBuf,unsigned char  BinLen,unsigned char  *ASCBuf)
{
	unsigned char  i;
	for(i=0;i<=BinLen;i++)
	{
		ASCBuf[i]=BinBuf[i]+0x30;
	}
}
/************** 将BIN字符串展开为BCD字符串**************************************
输入参数:
	BINBuf: BIN字符串(即ASC码-'0')
	BINLen: BIN字符串的长度
输出参数:
	BCDBuf: BCD字符串
返回值:	无
********************************************************************************/
void Bin2BCD(unsigned char  *BinBuf,unsigned char  BinLen,unsigned char  *BCDBuf)
{
    unsigned char  i,j,k,m;
    for(i=0,j=0;i<BinLen;i+=2)
    {  k=BinBuf[i]<<4;
	   m=BinBuf[i+1];
	   BCDBuf[j++]=k+m;
    }
}
//////////////////////////////////////////////////////////   
//功能：十六进制转为十进制   
//输入：const unsigned char *hex         待转换的十六进制数据   
//      int length                       十六进制数据长度   
//输出：   
//返回：int  rslt                        转换后的十进制数据   
//思路：十六进制每个字符位所表示的十进制数的范围是0 ~255，进制为256   
//      左移8位(<<8)等价乘以256   
/////////////////////////////////////////////////////////   
unsigned long Hex2Dec(const unsigned char *hex, int length)  
{  
    int i;  
    unsigned long rslt = 0;  
    for (i = 0; i < length; i++)  
    {  
        rslt += (unsigned long)(hex[i]) << (8 * (length - 1 - i));  
  
    }  
    return rslt;  
}  
  
/////////////////////////////////////////////////////////   
//功能：十进制转十六进制   
//输入：int dec						待转换的十进制数据   
//      int length					转换后的十六进制数据长度   
//输出：unsigned char *hex			转换后的十六进制数据   
//返回：0    success   
//思路：原理同十六进制转十进制   
//////////////////////////////////////////////////////////   
int Dec2Hex(int dec, unsigned char *hex, int length)  
{  
    int i;  
    for (i = length - 1; i >= 0; i--)  
    {  
        hex[i] = (dec % 256) & 0xFF;  
        dec /= 256;  
    }  
    return 0;  
}  
/////////////////////////////////////////////////////////   
//功能：BCD转10进制   
//输入：const unsigned char *bcd     待转换的BCD码   
//      int length                   BCD码数据长度   
//输出：   
//返回：unsigned long               当前数据位的权   
//思路：压缩BCD码一个字符所表示的十进制数据范围为0 ~ 99,进制为100   
//      先求每个字符所表示的十进制值，然后乘以权   
//////////////////////////////////////////////////////////   
unsigned long  BCD2Dec(const unsigned char *BCD, int length)  
{  
    int i, tmp;  
    unsigned long dec = 0;  
    for (i = 0; i < length; i++)  
    {  
        tmp = ((BCD[i] >> 4) & 0x0F) * 10 + (BCD[i] & 0x0F);  
        dec += tmp * power(100, length - 1 - i);  
    }  
    return dec;  
}  
/////////////////////////////////////////////////////////   
//功能：十进制转BCD码   
//输入：int Dec                      待转换的十进制数据   
//      int length                   BCD码数据长度   
//输出：unsigned char *Bcd           转换后的BCD码   
//返回：0  success   
//思路：原理同BCD码转十进制   
//////////////////////////////////////////////////////////   
int Dec2BCD(int Dec, unsigned char *BCD, int length)  
{  
    int i;  
    int temp;  
    for (i = length - 1; i >= 0; i--)  
    {  
        temp = Dec % 100;  
        BCD[i] = ((temp / 10) << 4) + ((temp % 10) & 0x0F);  
        Dec /= 100;  
    }  
    return 0;  
}  

void Hex2BCD(char* HEX, char* BCD)
{
	int i=0;
	while(HEX[i])
	{
		BCD[i]=(HEX[i]/10*16)+(HEX[i]%10);
		i++;
	}
	BCD[i]='\0';
}

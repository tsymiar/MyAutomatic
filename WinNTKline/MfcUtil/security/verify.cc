#include "verify.h"

//Hash的前8位与后8位进行异或产生8位字hash1.
unsigned char* get_hash1(unsigned char* ha,unsigned  char* sh, unsigned char* sa)
{
	int i=0;
	for(;i<8;i++)
		sa[i]=ha[i]^sh[i];
	return sa;
}

//用工作密钥对hash1进行加密运算(IDEA算法ECB)，结果为密押MIYA。
unsigned char* get_MIYA(struct KEYINFO workey, unsigned char* hash1, unsigned char* miya)
{
	IDEA_KEY_SCHEDULE* wk=0;
	idea_ecb_encrypt(hash1,miya,wk);
	return miya;
}

//读取工作密钥并用对方公钥加密，输出加密密文并导出。
unsigned char* get_Ciphertext(char* filename, char* workey, unsigned char* text)
{	// 打开公钥文件
	FILE* fp;
	int len;
	RSA* rsa1;
	//RSArefPubKey_st *pubkey;
    FILE* pub_fp=fopen(filename,"r");
    if(pub_fp==NULL){
        printf("failed to open pub_key file %s!\n", filename);
        return (unsigned char*)-1;
    }
    // 从文件中读取公钥
    rsa1=PEM_read_RSA_PUBKEY(pub_fp, NULL, NULL, NULL);
    if(rsa1==NULL){
        printf("unable to read public key!\n");
        return (unsigned char*)-1; 
    }
    if(strlen(workey)>=RSA_size(rsa1)-41){
        printf("failed to encrypt\n");
        return (unsigned char*)-1;
    }
    fclose(pub_fp);
    // 用公钥加密(PKCS#1)
    len=RSA_public_encrypt(strlen(workey), (unsigned char*)workey, text, rsa1, RSA_PKCS1_PADDING);
    if(len==-1 ){
        printf("failed to encrypt\n");
        return (unsigned char*)-1;
    }
    // 输出加密后的密文
    fp=fopen("out.txt","w");
    if(fp){
        fwrite(text,len,1,fp);
        fclose(fp);
    }
	return text;
}

//提取字符串头或尾8个字节
char* extra_8bt(char* prim, char* bt, int h)
{
	int i=0;
	if(h==1)
		for (;i<8;i++)
			bt[i]=(unsigned char)prim[i];
	else
		for (i=8;i<16;i++)
			bt[i-8]=(unsigned char)prim[i];
	return bt;
}

unsigned int get_CRC32(char* InStr,unsigned int len)
{        
  //生成Crc32的查询表     
  unsigned int Crc32Table[256];      
  int i,j;        
  unsigned int CRC;        
  for (i = 0; i < 256; i++){        
    CRC = i;        
    for (j = 0; j < 8; j++){        
      if (CRC & 1)        
        CRC = (CRC >> 1) ^ 0xEDB88320;        
      else       
        CRC >>= 1;      
    }        
    Crc32Table[i] = CRC;        
  }        
    
  //开始计算CRC32校验值     
  CRC=0xffffffff;        
  for(i=0; (unsigned int)i<len; i++){          
    CRC = (CRC >> 8) ^ Crc32Table[(CRC & 0xFF) ^ InStr[i]];        
  }     
       
  CRC ^= 0xFFFFFFFF;     
  return CRC;        
}        
    
unsigned short get_CRC16(char* InStr,unsigned int len)
{        
  //生成CRC16的查询表     
  unsigned short Crc16Table[256];      
  unsigned int i,j;     
  unsigned short CRC;        
  for (i = 0; i < 256; i++)  
  {       
    CRC = i;        
    for (j = 0; j < 8; j++)  
    {        
      if(CRC & 0x1)        
        CRC = (CRC >> 1) ^ 0xA001;        
      else       
        CRC >>= 1;      
    }        
    Crc16Table[i] = CRC;        
  }     
       
  //开始计算CRC16校验值     
  CRC=0x0000;          
  for(i=0; i<len; i++){          
    CRC = (CRC >> 8) ^ Crc16Table[(CRC & 0xFF) ^ InStr[i]];       
    
  }     
  //CRC ^= 0x0000;       
  return CRC;        
}  

unsigned long cal_CRC(unsigned long dwPolynomial, unsigned long *ptr, int len)
 {
     unsigned long    xbit;
     unsigned long    data;
     unsigned long    CRC = 0xFFFFFFFF;    // init
     int bits = 0;
     while (len--) {
         xbit = 1 << 31;
 
         data = *ptr++;
         for (bits = 0; bits < 32; bits++) {
             if (CRC & 0x80000000) {
                 CRC <<= 1;
                 CRC ^= dwPolynomial;
             }
             else
                 CRC <<= 1;
             if (data & xbit)
                 CRC ^= dwPolynomial;
 
             xbit >>= 1;
         }
     }
     return CRC;
 }

int get_CRC(unsigned char *data, int len, unsigned char CRC[2])
{
	int i;

	if (len % 2 != 0)
		return -1;

	memset(CRC, 0, 2);
	for (i = 0;i < len / 2;i++)
	{
		CRC[0] ^= data[i*2];
		CRC[1] ^= data[i*2+1];
	}
	return 0;
}

//字符串拼接
char * _strcat(unsigned char * strDest, unsigned char * strSrc)
{
	unsigned char * cp;
	if ((strDest == NULL) || (strSrc == NULL))
		return 0;//throw "buffer is empty";
	cp = strDest;
	while (*cp)
		cp++; /* find end of strDest */
	while ((*cp++ = *strSrc++)&&(*strSrc!= '\0')); /* Copy strSrc to end of strDest */
	*cp++='\0';
	return (char*)(strDest); /* return strDest */
}

//字符串复制
unsigned char* _strcpy(unsigned char* strDest, const char* strSrc, int N)
{
	unsigned char * cp;
	size_t available;
	unsigned char* strDestCopy = (unsigned char*)strSrc;
    cp = strDest;
    available = N;
    while ((*cp++ = *strSrc++) != 0 && --available > 0)
    {}
 
    if (available == 0)
    {
		
    }
	return strDestCopy;
}

void BCD_cut(unsigned char* TRUSTID,int cutlen, char* cut10)
{
    unsigned char  i,j,k,m;
    for(i=0,j=0;i<cutlen;i+=2)
    {  
		k=TRUSTID[i]<<4;
		m=TRUSTID[i+1];
		cut10[j++]=k+m;
    }
}

void BCD_exp(unsigned char* Ascbuf, int len, char* exp)
{
	unsigned char  i,j;
	for(i=len;i!=0;i--)
	{
		j=Ascbuf[i-1];
		exp[i*2-1]=j&0x0F;
		exp[i*2-2]=j>>4;
	}
	exp[len]='\0';
}

void RSA_check()
{
	//int ret;
	//int rv=0;
	FILE *fp=0;
	//RSA *key=0;
	//char msg2[256];
	char msg[]="i, i have no data to enc";
	RSArefPubKey_st PUBLIC_KEY;
	printf("%s",msg);
    fp = fopen("PublicRSA", "r");
    if(fp == NULL)
        printf("fp error!");
	//rv = read(fp, &PUBLIC_KEY, sizeof(RSArefPubKey_st));
    //key = PEM_read_RSAPublicKey(fp, NULL/*&key*/, NULL, NULL);
	//ret = RSA_public_encrypt(strlen(msg), (unsigned char *)msg, (unsigned char *)msg2, key, RSA_PKCS1_PADDING); // or RSA_PKCS1_OAEP_PADDING
	//if(RSA_verify(NID_idea_ecb,)==1)
	//	printf("验证成功\n");
}

int i=0;
char cc[2];
char BCD20[20]={0};
char Hex20[20];
char EBCD20[20];
char hash[16];
char hash1[8];
unsigned char ha[8];
unsigned char sh[8];
unsigned char CRC[2];
unsigned char ASC10[10]={0};

void Verify(char out[])
{  
	char code[]={0x05,0x05,0x06,0x01,0x04,0x0F,0x0D,0x0F,0x04,0x03,0x02,0x0B,0x0C,0x06,0x0B,0x06,0x09,0x0F,0x02,0x03};
	printf("32字节:\t%s\n16字节:\t\t%s\n",out,get_Hash(out, 32, hash));
	printf("Hash前8位:\t");
	extra_8bt(hash,(char*)ha,1);//提取前8字节
	for (i=0;i<8;i++)
		printf("%c",ha[i]);
	printf("\n");
	//CRC运算
	get_CRC(ha,8,CRC);
	hex_to_str((unsigned char*)CRC, cc);
	printf("\tCRC:%c(%02x),%c(%02x)\n",cc[0],cc[0],cc[1],cc[1]);
	printf("\t复制：	");
	_strcpy(ASC10,(char*)ha,8);
	for(i=0;i<8;i++)
		printf("%c",ASC10[i]);
	printf("\n");
	//拼接
	_strcat(ASC10,(unsigned char*)cc);
	printf("\t原标识：");
	for(i=0;i<10;i++)
		printf("%c",ASC10[i]);
	printf("\n\t	");
	for(i=0;i<10;i++)
		printf("%02x ",ASC10[i]);
	printf("\n");
	printf("\t中心标识：");
	//BCD扩展
	asc_to_hex((unsigned char*)Hex20,ASC10,20);
	for(i=0;i<10;i++)
		printf("%02X",(unsigned char)Hex20[i]);
	/*printf("\n\t按10进制输出：	");                                               
	hex_to_str((unsigned char*)Hex20,BCD20);
	printf("\n\t	");
	for(i=0;i<20;i++)
		printf("%c ",BCD20[i]);*/
	printf("\n");
	printf("\t定义一个中心标识：");
	for(i=0;i<20;i++)
		printf("%x",code[i]);
	printf("\n\t将TRUSTID转成10位BCD码:");
	///////////////////////////////////
 	//BCD_cut((unsigned char*)code,20,BCD10);
	BCD_cut((unsigned char*)code,20,EBCD20);
	for (i=0;i<10;i++) 
		printf ("%c",(unsigned char)EBCD20[i]);
	printf("\n");
	printf("Hash后8位：\t");
	//提取后8字节
	extra_8bt(hash,(char*)sh,0);
	for (i=0;i<8;i++)
		printf("%c",sh[i]);
	printf("\n");
	printf("异或产生8位字hash1：\t");
	get_hash1(ha,sh,(unsigned char*)hash1);
	for (i=0;i<8;i++)
                printf("%c",hash1[i]);
	printf("\t");
	for (i=0;i<8;i++)
		printf("%02x",(unsigned char)hash1[i]);
	printf("\n");
	//getchar();
}

void serify(char* str)
{
	int i=0;
	char dig[16]; 
	char out[32];
	md5_str(str,dig); 
	printf("摘要:\t"); 
	for (;i<16;i++) //16位无符号整数
		printf ("%02x",(unsigned char)dig[i]);
	printf("\n");
	Verify(out);
}

void ferify(char* filename)
{
	//RSAPublic读取
	FILE *fp;
	char out[32];
	if (filename[0]==34) filename[strlen(filename)-1]=0,strcpy(filename,filename+1);  //支持文件拖曳,但会多出双引号,这里是处理多余的双引号
	if (!strcmp(filename,"exit")) exit(0);  //输入exit退出
	if (!(fp=fopen(filename,"rb"))) {printf("Can not open this file!\n");return/*continue*/;}  //以二进制打开文件
	md5_file(fp, out);
	Verify(out);
}

#define MD5FILE

void test_verify()
{
	while(1)
	{
#ifdef MD5FILE
		printf("Input file:PublicRSA\n");
		ferify("PublicRSA");
	//	RSA_check();
#else
		char encrypt[1024];  
		printf("请输入要计算MD5值的字符串:");
		gets(encrypt);
		serify(encrypt);
#endif
	}
}

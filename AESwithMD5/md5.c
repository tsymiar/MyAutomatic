#include "md5.h"

typedef unsigned char *POINTER;
typedef unsigned short int uint2_t;
typedef unsigned long int uint4_t;

typedef struct
{
	uint4_t state[4];
	uint4_t count[2];
	unsigned char buffer[64];
} MD5_CTX;

unsigned A, B, C, D, a, b, c, d;   //i临时变量,len文件长,flen[2]为64位二进制表示的文件初始长度

static unsigned char PADDING[64] = {
	0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

#define F(x, y, z) (((x) & (y)) | ((~x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & (~z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | (~z)))

#define RL(x, y) (((x) << (y)) | ((x) >> (32 - (y))))  //x向左循环移y位

#define PP(x) (x<<24)|((x<<8)&0xff0000)|((x>>8)&0xff00)|(x>>24)  //将x高低位互换,例如PP(aabbccdd)=ddccbbaa 大端小端转化

#define FF(a, b, c, d, x, s, ac) a = b + (RL((a + F(b,c,d) + x + ac),s))
#define GG(a, b, c, d, x, s, ac) a = b + (RL((a + G(b,c,d) + x + ac),s))
#define HH(a, b, c, d, x, s, ac) a = b + (RL((a + H(b,c,d) + x + ac),s))
#define II(a, b, c, d, x, s, ac) a = b + (RL((a + I(b,c,d) + x + ac),s))

void md5_round(unsigned int x[])               //MD5核心算法,共64轮
{
/**//* Round 1 */
  FF (a, b, c, d, x[ 0],  7, 0xd76aa478); /**//* 1 */
  FF (d, a, b, c, x[ 1], 12, 0xe8c7b756); /**//* 2 */
  FF (c, d, a, b, x[ 2], 17, 0x242070db); /**//* 3 */
  FF (b, c, d, a, x[ 3], 22, 0xc1bdceee); /**//* 4 */
  FF (a, b, c, d, x[ 4],  7, 0xf57c0faf); /**//* 5 */
  FF (d, a, b, c, x[ 5], 12, 0x4787c62a); /**//* 6 */
  FF (c, d, a, b, x[ 6], 17, 0xa8304613); /**//* 7 */
  FF (b, c, d, a, x[ 7], 22, 0xfd469501); /**//* 8 */
  FF (a, b, c, d, x[ 8],  7, 0x698098d8); /**//* 9 */
  FF (d, a, b, c, x[ 9], 12, 0x8b44f7af); /**//* 10 */
  FF (c, d, a, b, x[10], 17, 0xffff5bb1); /**//* 11 */
  FF (b, c, d, a, x[11], 22, 0x895cd7be); /**//* 12 */
  FF (a, b, c, d, x[12],  7, 0x6b901122); /**//* 13 */
  FF (d, a, b, c, x[13], 12, 0xfd987193); /**//* 14 */
  FF (c, d, a, b, x[14], 17, 0xa679438e); /**//* 15 */
  FF (b, c, d, a, x[15], 22, 0x49b40821); /**//* 16 */

/**//* Round 2 */
  GG (a, b, c, d, x[ 1],  5, 0xf61e2562); /**//* 17 */
  GG (d, a, b, c, x[ 6],  9, 0xc040b340); /**//* 18 */
  GG (c, d, a, b, x[11], 14, 0x265e5a51); /**//* 19 */
  GG (b, c, d, a, x[ 0], 20, 0xe9b6c7aa); /**//* 20 */
  GG (a, b, c, d, x[ 5],  5, 0xd62f105d); /**//* 21 */
  GG (d, a, b, c, x[10],  9, 0x02441453); /**//* 22 */
  GG (c, d, a, b, x[15], 14, 0xd8a1e681); /**//* 23 */
  GG (b, c, d, a, x[ 4], 20, 0xe7d3fbc8); /**//* 24 */
  GG (a, b, c, d, x[ 9],  5, 0x21e1cde6); /**//* 25 */
  GG (d, a, b, c, x[14],  9, 0xc33707d6); /**//* 26 */
  GG (c, d, a, b, x[ 3], 14, 0xf4d50d87); /**//* 27 */
  GG (b, c, d, a, x[ 8], 20, 0x455a14ed); /**//* 28 */
  GG (a, b, c, d, x[13],  5, 0xa9e3e905); /**//* 29 */
  GG (d, a, b, c, x[ 2],  9, 0xfcefa3f8); /**//* 30 */
  GG (c, d, a, b, x[ 7], 14, 0x676f02d9); /**//* 31 */
  GG (b, c, d, a, x[12], 20, 0x8d2a4c8a); /**//* 32 */

  /**//* Round 3 */
  HH (a, b, c, d, x[ 5],  4, 0xfffa3942); /**//* 33 */
  HH (d, a, b, c, x[ 8], 11, 0x8771f681); /**//* 34 */
  HH (c, d, a, b, x[11], 16, 0x6d9d6122); /**//* 35 */
  HH (b, c, d, a, x[14], 23, 0xfde5380c); /**//* 36 */
  HH (a, b, c, d, x[ 1],  4, 0xa4beea44); /**//* 37 */
  HH (d, a, b, c, x[ 4], 11, 0x4bdecfa9); /**//* 38 */
  HH (c, d, a, b, x[ 7], 16, 0xf6bb4b60); /**//* 39 */
  HH (b, c, d, a, x[10], 23, 0xbebfbc70); /**//* 40 */
  HH (a, b, c, d, x[13],  4, 0x289b7ec6); /**//* 41 */
  HH (d, a, b, c, x[ 0], 11, 0xeaa127fa); /**//* 42 */
  HH (c, d, a, b, x[ 3], 16, 0xd4ef3085); /**//* 43 */
  HH (b, c, d, a, x[ 6], 23, 0x04881d05); /**//* 44 */
  HH (a, b, c, d, x[ 9],  4, 0xd9d4d039); /**//* 45 */
  HH (d, a, b, c, x[12], 11, 0xe6db99e5); /**//* 46 */
  HH (c, d, a, b, x[15], 16, 0x1fa27cf8); /**//* 47 */
  HH (b, c, d, a, x[ 2], 23, 0xc4ac5665); /**//* 48 */

  /**//* Round 4 */
  II (a, b, c, d, x[ 0],  6, 0xf4292244); /**//* 49 */
  II (d, a, b, c, x[ 7], 10, 0x432aff97); /**//* 50 */
  II (c, d, a, b, x[14], 15, 0xab9423a7); /**//* 51 */
  II (b, c, d, a, x[ 5], 21, 0xfc93a039); /**//* 52 */
  II (a, b, c, d, x[12],  6, 0x655b59c3); /**//* 53 */
  II (d, a, b, c, x[ 3], 10, 0x8f0ccc92); /**//* 54 */
  II (c, d, a, b, x[10], 15, 0xffeff47d); /**//* 55 */
  II (b, c, d, a, x[ 1], 21, 0x85845dd1); /**//* 56 */
  II (a, b, c, d, x[ 8],  6, 0x6fa87e4f); /**//* 57 */
  II (d, a, b, c, x[15], 10, 0xfe2ce6e0); /**//* 58 */
  II (c, d, a, b, x[ 6], 15, 0xa3014314); /**//* 59 */
  II (b, c, d, a, x[13], 21, 0x4e0811a1); /**//* 60 */
  II (a, b, c, d, x[ 4],  6, 0xf7537e82); /**//* 61 */
  II (d, a, b, c, x[11], 10, 0xbd3af235); /**//* 62 */
  II (c, d, a, b, x[ 2], 15, 0x2ad7d2bb); /**//* 63 */
  II (b, c, d, a, x[ 9], 21, 0xeb86d391); /**//* 64 */
}

void encode(unsigned char *output, uint4_t *input, unsigned int len)  
{  
  unsigned int i, j;  
  
  for (i = 0, j = 0; j < len; i++, j += 4) {  
  output[j] = (unsigned char)(input[i] & 0xff);  
  output[j+1] = (unsigned char)((input[i] >> 8) & 0xff);  
  output[j+2] = (unsigned char)((input[i] >> 16) & 0xff);  
  output[j+3] = (unsigned char)((input[i] >> 24) & 0xff);  
  }  
}  

void decode(uint4_t *output, unsigned char *input, unsigned int len)  
{  
  unsigned int i, j;  
  
  for (i = 0, j = 0; j < len; i++, j += 4)  
  output[i] = ((uint4_t)input[j]) | (((uint4_t)input[j+1]) << 8) |  
  (((uint4_t)input[j+2]) << 16) | (((uint4_t)input[j+3]) << 24);  
}  

void md5_trans(uint4_t st[4], unsigned char block[64])  
{  
  uint4_t a = st[0], b = st[1], c = st[2], d = st[3], x[16]; 
  decode (x, block, 64); 
/**//* Round 1 */
  FF (a, b, c, d, x[ 0],  7, 0xd76aa478); /**//* 1 */
  FF (d, a, b, c, x[ 1], 12, 0xe8c7b756); /**//* 2 */
  FF (c, d, a, b, x[ 2], 17, 0x242070db); /**//* 3 */
  FF (b, c, d, a, x[ 3], 22, 0xc1bdceee); /**//* 4 */
  FF (a, b, c, d, x[ 4],  7, 0xf57c0faf); /**//* 5 */
  FF (d, a, b, c, x[ 5], 12, 0x4787c62a); /**//* 6 */
  FF (c, d, a, b, x[ 6], 17, 0xa8304613); /**//* 7 */
  FF (b, c, d, a, x[ 7], 22, 0xfd469501); /**//* 8 */
  FF (a, b, c, d, x[ 8],  7, 0x698098d8); /**//* 9 */
  FF (d, a, b, c, x[ 9], 12, 0x8b44f7af); /**//* 10 */
  FF (c, d, a, b, x[10], 17, 0xffff5bb1); /**//* 11 */
  FF (b, c, d, a, x[11], 22, 0x895cd7be); /**//* 12 */
  FF (a, b, c, d, x[12],  7, 0x6b901122); /**//* 13 */
  FF (d, a, b, c, x[13], 12, 0xfd987193); /**//* 14 */
  FF (c, d, a, b, x[14], 17, 0xa679438e); /**//* 15 */
  FF (b, c, d, a, x[15], 22, 0x49b40821); /**//* 16 */

/**//* Round 2 */
  GG (a, b, c, d, x[ 1],  5, 0xf61e2562); /**//* 17 */
  GG (d, a, b, c, x[ 6],  9, 0xc040b340); /**//* 18 */
  GG (c, d, a, b, x[11], 14, 0x265e5a51); /**//* 19 */
  GG (b, c, d, a, x[ 0], 20, 0xe9b6c7aa); /**//* 20 */
  GG (a, b, c, d, x[ 5],  5, 0xd62f105d); /**//* 21 */
  GG (d, a, b, c, x[10],  9, 0x02441453); /**//* 22 */
  GG (c, d, a, b, x[15], 14, 0xd8a1e681); /**//* 23 */
  GG (b, c, d, a, x[ 4], 20, 0xe7d3fbc8); /**//* 24 */
  GG (a, b, c, d, x[ 9],  5, 0x21e1cde6); /**//* 25 */
  GG (d, a, b, c, x[14],  9, 0xc33707d6); /**//* 26 */
  GG (c, d, a, b, x[ 3], 14, 0xf4d50d87); /**//* 27 */
  GG (b, c, d, a, x[ 8], 20, 0x455a14ed); /**//* 28 */
  GG (a, b, c, d, x[13],  5, 0xa9e3e905); /**//* 29 */
  GG (d, a, b, c, x[ 2],  9, 0xfcefa3f8); /**//* 30 */
  GG (c, d, a, b, x[ 7], 14, 0x676f02d9); /**//* 31 */
  GG (b, c, d, a, x[12], 20, 0x8d2a4c8a); /**//* 32 */

  /**//* Round 3 */
  HH (a, b, c, d, x[ 5],  4, 0xfffa3942); /**//* 33 */
  HH (d, a, b, c, x[ 8], 11, 0x8771f681); /**//* 34 */
  HH (c, d, a, b, x[11], 16, 0x6d9d6122); /**//* 35 */
  HH (b, c, d, a, x[14], 23, 0xfde5380c); /**//* 36 */
  HH (a, b, c, d, x[ 1],  4, 0xa4beea44); /**//* 37 */
  HH (d, a, b, c, x[ 4], 11, 0x4bdecfa9); /**//* 38 */
  HH (c, d, a, b, x[ 7], 16, 0xf6bb4b60); /**//* 39 */
  HH (b, c, d, a, x[10], 23, 0xbebfbc70); /**//* 40 */
  HH (a, b, c, d, x[13],  4, 0x289b7ec6); /**//* 41 */
  HH (d, a, b, c, x[ 0], 11, 0xeaa127fa); /**//* 42 */
  HH (c, d, a, b, x[ 3], 16, 0xd4ef3085); /**//* 43 */
  HH (b, c, d, a, x[ 6], 23, 0x04881d05); /**//* 44 */
  HH (a, b, c, d, x[ 9],  4, 0xd9d4d039); /**//* 45 */
  HH (d, a, b, c, x[12], 11, 0xe6db99e5); /**//* 46 */
  HH (c, d, a, b, x[15], 16, 0x1fa27cf8); /**//* 47 */
  HH (b, c, d, a, x[ 2], 23, 0xc4ac5665); /**//* 48 */

  /**//* Round 4 */
  II (a, b, c, d, x[ 0],  6, 0xf4292244); /**//* 49 */
  II (d, a, b, c, x[ 7], 10, 0x432aff97); /**//* 50 */
  II (c, d, a, b, x[14], 15, 0xab9423a7); /**//* 51 */
  II (b, c, d, a, x[ 5], 21, 0xfc93a039); /**//* 52 */
  II (a, b, c, d, x[12],  6, 0x655b59c3); /**//* 53 */
  II (d, a, b, c, x[ 3], 10, 0x8f0ccc92); /**//* 54 */
  II (c, d, a, b, x[10], 15, 0xffeff47d); /**//* 55 */
  II (b, c, d, a, x[ 1], 21, 0x85845dd1); /**//* 56 */
  II (a, b, c, d, x[ 8],  6, 0x6fa87e4f); /**//* 57 */
  II (d, a, b, c, x[15], 10, 0xfe2ce6e0); /**//* 58 */
  II (c, d, a, b, x[ 6], 15, 0xa3014314); /**//* 59 */
  II (b, c, d, a, x[13], 21, 0x4e0811a1); /**//* 60 */
  II (a, b, c, d, x[ 4],  6, 0xf7537e82); /**//* 61 */
  II (d, a, b, c, x[11], 10, 0xbd3af235); /**//* 62 */
  II (c, d, a, b, x[ 2], 15, 0x2ad7d2bb); /**//* 63 */
  II (b, c, d, a, x[ 9], 21, 0xeb86d391); /**//* 64 */
  
  st[0] += a;  
  st[1] += b;  
  st[2] += c;  
  st[3] += d;  
  memset ((POINTER)x, 0, sizeof (x));  
}  
 
void md5_init(MD5_CTX *context)  
{  
  context->count[0] = context->count[1] = 0;  
  context->state[0] = 0x67452301;  
  context->state[1] = 0xefcdab89;  
  context->state[2] = 0x98badcfe;  
  context->state[3] = 0x10325476;  
}  

void md5_update(MD5_CTX *context, unsigned char *input, unsigned int inputLen)  
{  
  unsigned int i, index, partLen;  
  
  index = (unsigned int)((context->count[0] >> 3) & 0x3F);  
  if ((context->count[0] += ((uint4_t)inputLen << 3))  
  < ((uint4_t)inputLen << 3))  
  context->count[1]++;  
  context->count[1] += ((uint4_t)inputLen >> 29);  
  
  partLen = 64 - index;  
  
  if (inputLen >= partLen) {  
	  memcpy((POINTER)&context->buffer[index], (POINTER)input, partLen);  
	  md5_trans(context->state, context->buffer);  
	  
	  for (i = partLen; i + 63 < inputLen; i += 64)  
	  md5_trans(context->state, &input[i]);  
	  index = 0;  
  }  
  else  
	i = 0;  
  
  memcpy((POINTER)&context->buffer[index], (POINTER)&input[i], inputLen-i);  
}  

void md5_final(unsigned char digest[16], MD5_CTX *context)  
{  
  unsigned char bits[8];  
  unsigned int index, padLen;  
  
  encode (bits, context->count, 8);  
  index = (unsigned int)((context->count[0] >> 3) & 0x3f);  
  padLen = (index < 56) ? (56 - index) : (120 - index);  
  md5_update (context, PADDING, padLen);  
  md5_update (context, bits, 8);  
  encode (digest, context->state, 16);  
  memset ((POINTER)context, 0, sizeof (*context));  
}  


void md5_f_set(unsigned int x[])
{  
  a=A,b=B,c=C,d=D;
  md5_round(x);
  A += a;
  B += b;
  C += c;
  D += d;
}

void md5_file(FILE *fp,char out[])
{
	int len;
	unsigned i, flen[2], x[16];
	if(!fp)return;
	fseek(fp, 0, SEEK_END);  //文件指针转到文件末尾
	if ((len = ftell(fp)) == -1) { printf("Sorry! Can not calculate files which larger than 2 GB!\n"); fclose(fp); /*continue;*/ }  //ftell函数返回long,最大为2GB,超出返回-1
	rewind(fp);  //文件指针复位到文件头
	A = 0x67452301, B = 0xefcdab89, C = 0x98badcfe, D = 0x10325476; //初始化链接变量
	flen[1] = len / 0x20000000;     //flen单位是bit
	flen[0] = (len % 0x20000000) * 8;
	memset(x, 0, 64);   //初始化x数组为0
	fread(&x, 4, 16, fp);  //以4字节为一组,读取16组数据
	for (i = 0; i<(unsigned)len / 64; i++) {    //循环运算直至文件结束
		md5_f_set(x);
		memset(x, 0, 64);
		fread(&x, 4, 16, fp);
	}
	((char*)x)[len % 64] = 128;  //文件结束补1,补0操作,128二进制即10000000
	if (len % 64>55) md5_f_set(x), memset(x, 0, 64);
	memcpy(x + 14, flen, 8);    //文件末尾加入原文件的bit长度
	md5_f_set(x);
	fclose(fp);
	sprintf(out, /*sizeof(out),*/ "%08x%08x%08x%08x", PP(A), PP(B), PP(C), PP(D));  //高低位逆反输出
	printf("MD5Code:%s\n", out);
}

void md5_str(char *input, char *output)
{
	MD5_CTX context;
	unsigned int len = strlen(input);
	md5_init(&context);
	md5_update(&context, (unsigned char *)input, len);
	md5_final((unsigned char *)output, &context);
}


/*从32字节的MD5字符串的中间截取16个字符*/  
char* get_Hash(char *md5, char *dst)
{  
    char *in = md5;  
    char *out = dst;  
	int m=16;
    int len = strlen(md5);  
    if(m>len) m = len-8;
    in += 8;  
    while(m--) *(out++) = *(in++);  
    *(out++)='\0';  
    return dst;  
} 

void test_s_md5()
{
	char out[64];
	char digest[16];  
	char encrypt[200];  
	while(1)
	{
	  printf("请输入要计算MD5值的字符串:\n");
	  gets(encrypt);
	  printf("摘要结果:\n"); 
	  md5_str(encrypt,digest);  
	  hex_to_str((unsigned char*)digest,out);
	  printf("32字节：%s\n",out);
	  printf("16字节：\t%s\n",get_Hash(out, out));
	  //getchar();
	}
}

void test_f_md5()
{
	char filename[200];   //文件名
	char out[128];
	FILE *fp;
	while(1){
		printf("Input file:");
		fgets(filename,1024,fp);    //用get函数,避免scanf以空格分割数据,
		if (filename[0]==34) filename[strlen(filename)-1]=0,strcpy(filename,filename+1);  //支持文件拖曳,但会多出双引号,这里是处理多余的双引号
		if (!strcmp(filename,"exit")) exit(0);  //输入exit退出
		if (!(fp=fopen(filename,"rb"))) {printf("Can not open this file!\n");/*continue;*/}  //以二进制打开文件
		md5_file(fp, out);
  }
}

// http://msdn.microsoft.com/en-us/library/hskdteyh.aspx
#ifndef _CPUID_H
#define _CPUID_H
#pragma warning (disable:4793)
#include <Windows.h>
#if _MSC_VER >= 1400//VC2005+支持intrin.h
#include <intrin.h>
#endif
#if defined(_WIN64)
//64位不支持内联汇编，应使用__cpuid、__cpuidex等intrinsics函数
#else 
#ifdef  __cplusplus
extern "C" {
#endif //  __cplusplus
#ifndef NULL
#define NULL 0
#endif // !NULL
#if _MSC_VER<1600	//VS2010.据说VC2008 SP1后支持__cpuidex
	inline void __cpuidex(__int32 CPUInfo[4], __int32 InfoType, __int32 ECXValue)
	{
		if (CPUInfo == NULL)
			return;
		_asm {
			//读取参数到寄存器
			move edi, CPUInfo;//edi寻址
			move eax, InfoType;
			move ecx, ECXValue;
			//CPUID
		cpuid:
			//save.CPUInfo
			move[edi], eax;
			move[edi + 4], ebx;
			move[edi + 8], ecx;
			move[edi + 12], edx;
		}
	}
#endif
#if _MSC_VER < 1400
	inline void __cpuid(__int32 CPUInfo[4], __int32 InfoType)
	{
		__cpuidex(CPUInfo, InfoType, 0);
	}
#endif // _MSC_VER < 1400
#endif // #if defined(_WIN64)

	// 取得CPU厂商（Vendor）
	inline int cpu_getvendor(char* pvendor/*厂商信息缓冲区至少为13字节*/)
	{
		INT32 dwBuf[4];
		if (NULL == pvendor)	return 0;
		// Function 0: Vendor-ID and Largest Standard Function
		__cpuid(dwBuf, 0);
		// save. 保存到pvendor
		*(INT32*)&pvendor[0] = dwBuf[1];	// ebx: 前面四个字符
		*(INT32*)&pvendor[4] = dwBuf[3];	// edx: 中间四个字符
		*(INT32*)&pvendor[8] = dwBuf[2];	// ecx: 最后四个字符
		pvendor[12] = '\0';
		return 12;//字符串的长度（一般为12）
	}

	// 取得CPU商标（Brand）
	inline int cpu_getbrand(char* pbrand/*商标信息的字符串缓冲区。至少为49字节*/)
	{
		INT32 dwBuf[4];
		if (NULL == pbrand)	return 0;
		// Function 0x80000000: Largest Extended Function Number
		__cpuid(dwBuf, 0x80000000);
		if (dwBuf[0] < 0x80000004)	return 0;
		// Function 80000002h,80000003h,80000004h: Processor Brand String
		__cpuid((INT32*)&pbrand[0], 0x80000002);	// 前面16个字符
		__cpuid((INT32*)&pbrand[16], 0x80000003);	// 中间16个字符
		__cpuid((INT32*)&pbrand[32], 0x80000004);	// 最后16个字符
		pbrand[48] = '\0';
		return 48;//字符串的长度（一般为48）
	}
#ifdef  __cplusplus
}
#endif
#endif // ! _CPUID_H

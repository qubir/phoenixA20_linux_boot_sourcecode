/*
************************************************************************************************************************
*                                                         eGON
*                                         the Embedded GO-ON Bootloader System
*
*                             Copyright(C), 2006-2008, SoftWinners Microelectronic Co., Ltd.
*											       All Rights Reserved
*
* File Name : check.c
*
* Author : Gary.Wang
*
* Version : 1.1.0
*
* Date : 2007.10.12
*
* Description : This file provides a function to check Boot0 and Boot1.
*
* Others : None at present.
*
*
* History :
*
*  <Author>        <time>       <version>      <description>
*
* Gary.Wang       2007.10.12      1.1.0        build the file
*
************************************************************************************************************************
*/

#include  "check_sum.h"

typedef struct tag_CRC32_DATA
{
	unsigned int CRC;				//int的大小是32位
	unsigned int CRC_32_Tbl[256];	//用来保存码表
}CRC32_DATA_t;
//#pragma arm section  code="check_magic"
/********************************************************************************
*函数名称: check_magic
*函数原型: __s32 check_magic( __u32 *mem_base, const char *magic )
*函数功能: 使用“算术和”来校验内存中的一段数据
*入口参数: mem_base       Boot文件在内存中的起始地址
*          magic          Boot的magic
*返 回 值: CHECK_IS_CORRECT      校验正确
*          CHECK_IS_WRONG        校验错误
*备    注:
********************************************************************************/
__s32 check_magic( __u32 *mem_base, const char *magic )
{
	__u32 i;
	boot_file_head_t *bfh;
	__u32 sz;
	unsigned char *p;


	bfh = (boot_file_head_t *)mem_base;
	p = bfh->magic;
	for( i = 0, sz = sizeof( bfh->magic );  i < sz;  i++ )
	{
		if( *p++ != *magic++ )
			return CHECK_IS_WRONG;
	}


	return CHECK_IS_CORRECT;
}

//#pragma arm section




//#pragma arm section  code="check_sum"
/********************************************************************************
*函数名称: check_sum
*函数原型: __s32 check_sum( __u32 *mem_base, __u32 size, const char *magic )
*函数功能: 使用“算术和”来校验内存中的一段数据
*入口参数: mem_base           待校验的数据在内存中的起始地址（必须是4字节对齐的）
*          size               待校验的数据的个数（以字节为单位，必须是4字节对齐的）
*返 回 值: CHECK_IS_CORRECT   校验正确
*          CHECK_IS_WRONG     校验错误
*备    注:
********************************************************************************/
__s32 check_sum( __u32 *mem_base, __u32 size )
{
	__u32 *buf;
	__u32 count;
	__u32 src_sum;
	__u32 sum;
	boot_file_head_t  *bfh;


	bfh = (boot_file_head_t *)mem_base;

	/* 生成校验和 */
	src_sum = bfh->check_sum;                  // 从Boot_file_head中的“check_sum”字段取出校验和
	bfh->check_sum = STAMP_VALUE;              // 将STAMP_VALUE写入Boot_file_head中的“check_sum”字段

	count = size >> 2;                         // 以 字（4bytes）为单位计数
	sum = 0;
	buf = (__u32 *)mem_base;
	do
	{
		sum += *buf++;                         // 依次累加，求得校验和
		sum += *buf++;                         // 依次累加，求得校验和
		sum += *buf++;                         // 依次累加，求得校验和
		sum += *buf++;                         // 依次累加，求得校验和
	}while( ( count -= 4 ) > (4-1) );

	while( count-- > 0 )
		sum += *buf++;

	bfh->check_sum = src_sum;                  // 恢复Boot_file_head中的“check_sum”字段的值

	if( sum == src_sum )
		return CHECK_IS_CORRECT;               // 校验成功
	else
		return CHECK_IS_WRONG;                 // 校验失败
}

//#pragma arm section

#define  ADD_SUM_MAGIC	0x62332672

/****************************************************************************************************/
/*  -----------------------------     -----------------------------                  								*/
/*  |  S4  | S5   | S6   | S7   |     |  S8  | S9   | S10   | S11 |                  								*/
/*  -----------------------------     -----------------------------                  								*/
/*      +     +      +      +            +      +      +       +                     								*/
/*  -----------------------------     -----------------------------                  								*/
/*  |  S12 | S13  | S14  | S15  |     |  S16 | S17  | S18  | S19  |                  								*/
/*  -----------------------------     -----------------------------                  								*/
/*                |                                 |                                								*/
/*               \|/                               \|/                               								*/
/*  -----------------------------     -----------------------------                  								*/
/*  |  S12 | S13  | S14  | S15  |     |  S16 | S17  | S18  | S19  |                  								*/
/*  -----------------------------     -----------------------------                  								*/
/*                \                                 /                                								*/
/*                 \                               /                                 								*/
/*                  \              +              /                                  								*/
/*                   \                           /                                   								*/
/*                    \                         /                                    								*/
/*                   -----------------------------     ----------------------------- 								*/
/*                   |  S16 | S17  | S18  | S19  |     |  S0  |  S1   | S2   | S3  | 								*/
/*                   -----------------------------     ----------------------------- 								*/          
/*                                  \                                 /              								*/
/*                                   \               +               /               								*/
/*                                    \                             /                								*/
/*                                     \                           /                 								*/
/*                                       -----------------------------               								*/
/*                                       |  S0  | S1   | S2   | S3   |               								*/
/*                                       -----------------------------               								*/
/*                                          \       /     \       /                  								*/
/*                                           \  +  /       \  +  /                   								*/
/*                                            \   /         \   /                    								*/
/*                                       -----------------------------               								*/
/*                                       |      S0    |    S1        |               								*/
/*                                       -----------------------------                                              */
/*                                                \          /                                                      */
/*                                                 \   +    /                                                       */
/*                                                  \      /                                                        */
/*                                                ------------               								        */
/*                                                |    S0    |              								        */
/*                                                ------------                                                      */
/*                                                                                   					     		*/
/*********************************************************************************************************************/
#pragma GCC push_options
#pragma GCC optimize ("O3")

__s32 verify_sum( void *mem_base, __u32 size )
{
#ifndef __NEON_SIMD__
	__u32 *buf;
	__u32 count;
	__u32 sum;

	count = size >> 2;                         // 以 字（4bytes）为单位计数
	sum = 0;
	buf = (__u32 *)mem_base;
	while(count--)
	{
		sum += *buf++;                         // 依次累加，求得校验和
	};

	switch(size & 0x03)
	{
		case 0:
			return sum;
		case 1:
			sum += (*buf & 0x000000ff);
			break;
		case 2:
			sum += (*buf & 0x0000ffff);
			break;
		case 3:
			sum += (*buf & 0x00ffffff);
			break;
	}

	return sum;
#else
__asm__ __volatile__ ("mov.w r2, %0 \n\t"::"r"(mem_base):"r2");
__asm__ __volatile__ ("lsrs.w r3, %0, #2\n\t"::"r"(size):"r3");

__asm__ __volatile__ (

	"stmdb	sp!, {r4, r5, r6, r8}\n\t"   //save the reg we will uses
	"movs	r4, #0       \n\t"           //r4 = clear the sum  
	"cmp	r3, #15      \n\t"          // data_len <=15 ?
	"and.w	ip, r1, #3   \n\t"          //ip/r12 = broken byte for checksum len @interlock
	"bls.n	broken_word_loop\n\t"       //if data_len <=15 word , go out the loop, proccess the rest broken word.
	"mov	r5, r3\n\t"                //r5 = data_len in word
    "vmov.I32 Q0, #0\n\t"             //clear Q0 to zero here...
    
"add_loop: \n\t"	

    "vld1.32 {d2,d3,d4,d5}, [r2]!\n\t"   //load Q1/Q2

    "sub	r5, #16      \n\t"      //r5 = data_len in word,data_len-=16,@interlock

    "vld1.32 {d6, d7,d8,d9}, [r2]!\n\t"   //load Q3/Q4
    
    "vadd.I32 Q2,Q1, Q2 \n\t"             //Q2 = Q1+Q2, total 8  vector word added.@interlock

    "vadd.I32 Q4, Q3, Q4 \n\t"            //Q4 = Q3+Q4, total 8  vector word added.@interlock

    "cmp	r5, #15      \n\t"           //if data_len in word >=16,we contiune the loop. 

    "vadd.I32 Q4, Q2, Q4 \n\t"           //add  two 16 vector to Q5
    
    "pld [r2]            \n\t"  
    
    "vadd.I32 Q0, Q0, Q4 \n\t"          //vector sum(128bit) += 16  vector word
	   
	"bhi.n	add_loop     \n\t"     //continue the loop ?
	
	//calculate the 128bit sum
    "vadd.I32 D0, D0, D1 \n\t"    //the sum is in S0/S1
    "vmov r4,r8 ,D0\n\t"        
    "add  r4, r4,r8\n\t"        //add sum to r4
    
	"sub.w	r2, r3, #16  \n\t"     //r2= r3(data_len) -=16                               
	"lsrs	r2, r2, #4   \n\t"     //how many 16 words in data_len ?                       
	"adds	r2, #1	     \n\t"
	"add.w	r0, r0, r2, lsl #6 \n\t"   //r0 = the new pbuf addr, indicate we have add 16 word,incrase the addr.
	"and.w	r3, r3, #15  \n\t"         //r3 = the broken word that less than 16 word 

"broken_word_loop:\n\t"
	"cbz	r3, broken_byte_loop\n\t"      //if the broken word is zero,go rest_loop_out    
    "mov    r2, r3             \n\t"       //r2 = the broken word that less than 16 word 
    "mov	r5, r0             \n\t"       //r0 = current buffer addr.
"broken_word_loop0:"
	"ldr.w	r6, [r5], #4       \n\t"      // r6 = *(addr), addr+=4
	"subs	r2, #1             \n\t"      // r2 = rest data_len -1 
	"add	r4, r6             \n\t"      // r4 =sum(r4)+ r6(*addr) @interlock
	"bne.n	broken_word_loop0  \n\t"      //if rest data_len in word >0 contiune the rest_loop
	"add.w	r0, r0, r3, lsl #2 \n\t"      // increase the  addr by the count of rest data_len in byte.

"broken_byte_loop:\n\t"
	"cmp.w	ip, #0  \n\t"    				//check if ip/r12 = broken byte for checksum  equ zero
	"beq.n	sum_exit\n\t"    				//if we have no broken byte for checksum,then,prepare go out.
	"cmp.w	ip, #1  \n\t"    				//if ip/r12 = broken byte for checksum  equ one
	"beq.n	add_broken_byte_one \n\t" 		//add one byte to sum
	"cmp.w	ip, #2 \n\t"     			    //if ip/r12 = broken byte for checksum  equ two
	"itet	ne \n\t"                 	 //if ip/r2 !=2 ,indicate broken byte is 3.so go next instraction
	"ldrne	r3, [r0, #0]\n\t"    		// indicate broken byte is 3.so load the word to r3.
	"ldrheq	r3, [r0, #0]\n\t"    		// indicate broken byte is 2, so load half word to r3.
	"bicne.w	r3, r3, #0xff000000	\n\t"//indicate borken byte is 3. clear the high 8bit.#4278190080
	"adds	r4, r4, r3 \n\t"        		 //add rest sum(r3) to sum(r4)
    "b      sum_exit\n\t"

"add_broken_byte_one:\n\t"
	"ldrb	r3, [r0, #0]\n\t"    // load one byte to r3
	"adds	r4, r4, r3  \n\t"    //r4 = r4(sum) + r3,

"sum_exit:\n\t"
	"mov	r0, r4 \n\t"     //save sum to r0
	"ldmia.w	sp!, {r4, r5, r6, r8}\n\t"
);

#endif
}
#pragma GCC pop_options

//#pragma arm section  code="check_file"
/********************************************************************************
*函数名称: check_file
*函数原型: __s32 check_file( __u32 *mem_base, __u32 size, const char *magic )
*函数功能: 使用“算术和”来校验内存中的一段数据
*入口参数: mem_base       待校验的数据在内存中的起始地址（必须是4字节对齐的）
*          size           待校验的数据的个数（以字节为单位，必须是4字节对齐的）
*          magic          magic number, 待校验文件的标识码
*返 回 值: CHECK_IS_CORRECT       校验正确
*          CHECK_IS_WRONG         校验错误
*备    注:
********************************************************************************/
__s32 check_file( __u32 *mem_base, __u32 size, const char *magic )
{
	if( check_magic( mem_base, magic ) == CHECK_IS_CORRECT
        &&check_sum( mem_base, size  ) == CHECK_IS_CORRECT )
        return CHECK_IS_CORRECT;
    else
    	return CHECK_IS_WRONG;
}
/*
************************************************************************************************************
*
*                                             _calc_crc32
*
*    函数名称：
*
*    参数列表：
*
*    返回值  ：
*
*    说明    ：
*
*
************************************************************************************************************
*/
__u32 calc_crc32(void * buffer, __u32 length)
{
	__u32 i, j;
	CRC32_DATA_t crc32;		//
	__u32 CRC32 = 0xffffffff; //设置初始值

	crc32.CRC = 0;

	for( i = 0; i < 256; ++i)//用++i以提高效率
	{
		crc32.CRC = i;
		for( j = 0; j < 8 ; ++j)
		{
			//这个循环实际上就是用"计算法"来求取CRC的校验码
			if(crc32.CRC & 1)
				crc32.CRC = (crc32.CRC >> 1) ^ 0xEDB88320;
			else //0xEDB88320就是CRC-32多项表达式的值
				crc32.CRC >>= 1;
		}
		crc32.CRC_32_Tbl[i] = crc32.CRC;
	}

	CRC32 = 0xffffffff; //设置初始值
    for( i = 0; i < length; ++i)
    {
        CRC32 = crc32.CRC_32_Tbl[(CRC32^((unsigned char*)buffer)[i]) & 0xff] ^ (CRC32>>8);
    }

    //return CRC32;
	return CRC32^0xffffffff;
}

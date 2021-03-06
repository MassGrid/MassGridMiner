/*
 * Copyright 2010-2011 Jeff Garzik
 * Copyright 2002-2010 Wei Dai (released as public domain)
 * Copyright 2012-2013 Luke Dashjr
 * Copyright 2011 Con Kolivas
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 3 of the License, or (at your option)
 * any later version.  See COPYING for more details.
 */

#include "config.h"

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "crypto/SHA256Digest.h"
#include "driver-cpu.h"
#include "miner.h"


#include "crypto/scrypt.h"
#include "crypto/sph_blake.h"
#include "crypto/sph_bmw.h"
#include "crypto/sph_groestl.h"
#include "crypto/sph_jh.h"
#include "crypto/sph_keccak.h"
#include "crypto/sph_skein.h"
#include "crypto/sph_luffa.h"
#include "crypto/sph_cubehash.h"
#include "crypto/sph_shavite.h"
#include "crypto/sph_simd.h"
#include "crypto/sph_echo.h"
#include "crypto/sph_hamsi.h"
#include "crypto/sph_fugue.h"
typedef uint32_t word32;

static word32 rotrFixed(word32 word, unsigned int shift)
{
	return (word >> shift) | (word << (32 - shift));
}

#define blk0(i) (W[i] = data[i])

static const word32 SHA256_K[64] = {
	0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5,
	0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
	0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3,
	0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
	0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc,
	0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
	0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7,
	0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
	0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13,
	0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
	0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3,
	0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
	0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5,
	0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
	0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208,
	0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};
#define blk2(i) (W[i&15]+=s1(W[(i-2)&15])+W[(i-7)&15]+s0(W[(i-15)&15]))

#define Ch(x,y,z) (z^(x&(y^z)))
#define Maj(x,y,z) (y^((x^y)&(y^z)))

#define a(i) T[(0-i)&7]
#define b(i) T[(1-i)&7]
#define c(i) T[(2-i)&7]
#define d(i) T[(3-i)&7]
#define e(i) T[(4-i)&7]
#define f(i) T[(5-i)&7]
#define g(i) T[(6-i)&7]
#define h(i) T[(7-i)&7]

#define R(i) h(i)+=S1(e(i))+Ch(e(i),f(i),g(i))+SHA256_K[i+j]+(j?blk2(i):blk0(i));\
	d(i)+=h(i);h(i)+=S0(a(i))+Maj(a(i),b(i),c(i))

// for SHA256
#define S0(x) (rotrFixed(x,2)^rotrFixed(x,13)^rotrFixed(x,22))
#define S1(x) (rotrFixed(x,6)^rotrFixed(x,11)^rotrFixed(x,25))
#define s0(x) (rotrFixed(x,7)^rotrFixed(x,18)^(x>>3))
#define s1(x) (rotrFixed(x,17)^rotrFixed(x,19)^(x>>10))

static void SHA256_Transform(word32 *state, const word32 *data)
{
	word32 W[16] = { };
	word32 T[8];
	unsigned int j;

    /* Copy context->state[] to working vars */
	memcpy(T, state, sizeof(T));
    /* 64 operations, partially loop unrolled */
	for (j=0; j<64; j+=16)
	{
		R( 0); R( 1); R( 2); R( 3);
		R( 4); R( 5); R( 6); R( 7);
		R( 8); R( 9); R(10); R(11);
		R(12); R(13); R(14); R(15);
	}
    /* Add the working vars back into context.state[] */
    state[0] += a(0);
    state[1] += b(0);
    state[2] += c(0);
    state[3] += d(0);
    state[4] += e(0);
    state[5] += f(0);
    state[6] += g(0);
    state[7] += h(0);
}

static void runhash(void *state, const void *input, const void *init)
{
	memcpy(state, init, 32);
	SHA256_Transform(state, input);
}
void Hex2Str(unsigned char *sSrc, unsigned char *sDest, int nSrcLen )  
{  
    int  i;  
	unsigned char szTmp[3];  
  
    for( i = 0; i < nSrcLen; i++ )  
    {  
        sprintf( szTmp, "%02X", (unsigned char) sSrc[i] );  
        memcpy( &sDest[i * 2], szTmp, 2 );  
	}
	sDest[i<<1]=0;  
    return ;  
}
void Hex2byte(unsigned char *sSrc,unsigned char *sDest,int len)
{
	for(int i=0;i<len/2;i++)
	{
		unsigned char temp2=0;
		if(sSrc[2*i]>=0x30&&sSrc[2*i]<=0x39)
		temp2|=((sSrc[2*i]-0x30)<<4);
		else if(sSrc[2*i]>=0x41&&sSrc[2*i]<=0x46)
		temp2|=((sSrc[2*i]-0x37)<<4);
		else
		temp2|=((sSrc[2*i]-0x59)<<4);
		if(sSrc[2*i+1]>=0x30&&sSrc[2*i+1]<=0x39)
		temp2|=(sSrc[2*i+1]-0x30);
		else if(sSrc[2*i+1]>=0x41&&sSrc[2*i+1]<=0x46)
		temp2|=(sSrc[2*i+1]-0x37);
		else
		temp2|=(sSrc[2*i+1]-0x59);
		sDest[i]=temp2;
	}

}
void compute(int id,const unsigned char * input, unsigned char * hash)
{
	switch (id)
	{
	case 0:{sph_blake512_context     ctx_blake;
		sph_blake512_init(&ctx_blake);
		sph_blake512(&ctx_blake, input, 32);
		sph_blake512_close(&ctx_blake, hash);
		break;
	}
	case 1:{sph_bmw512_context       ctx_bmw;
		sph_bmw512_init(&ctx_bmw);
		sph_bmw512(&ctx_bmw, input, 32);
		sph_bmw512_close(&ctx_bmw, hash);
		break; }
	case 2:{sph_groestl512_context   ctx_groestl;
	sph_groestl512_init(&ctx_groestl);
		sph_groestl512(&ctx_groestl, input, 32);
		sph_groestl512_close(&ctx_groestl, hash);
		break; }
	case 3:{sph_skein512_context     ctx_skein;
	sph_skein512_init(&ctx_skein);
		sph_skein512(&ctx_skein, input, 32);
		sph_skein512_close(&ctx_skein, hash);
		break; }
	case 4:{sph_jh512_context        ctx_jh;
	sph_jh512_init(&ctx_jh);
		sph_jh512(&ctx_jh, input, 32);
		sph_jh512_close(&ctx_jh, hash);
		break; }
	case 5:{sph_keccak512_context    ctx_keccak;
	sph_keccak512_init(&ctx_keccak);
		sph_keccak512(&ctx_keccak, input, 32);
		sph_keccak512_close(&ctx_keccak, hash);
		break; }
	case 6:{sph_luffa512_context	ctx_luffa1;
	sph_luffa512_init(&ctx_luffa1);
		sph_luffa512(&ctx_luffa1, input, 32);
		sph_luffa512_close(&ctx_luffa1, hash);
		break; }
	case 7:{sph_cubehash512_context	ctx_cubehash1;
	sph_cubehash512_init(&ctx_cubehash1);
		sph_cubehash512(&ctx_cubehash1, input, 32);
		sph_cubehash512_close(&ctx_cubehash1, hash);
		break; }
	case 8:{sph_shavite512_context	ctx_shavite1;
	sph_shavite512_init(&ctx_shavite1);
		sph_shavite512(&ctx_shavite1, input, 32);
		sph_shavite512_close(&ctx_shavite1, hash);
		break; }
	case 9:{sph_simd512_context		ctx_simd1;
	sph_simd512_init(&ctx_simd1);
		sph_simd512(&ctx_simd1, input, 32);
		sph_simd512_close(&ctx_simd1, hash);
		break; }
	case 10:{sph_echo512_context		ctx_echo1;
	sph_echo512_init(&ctx_echo1);
		sph_echo512(&ctx_echo1, input, 32);
		sph_echo512_close(&ctx_echo1, hash);
		break; }
	case 11:{sph_hamsi512_context	ctx_hamsi1;
	sph_hamsi512_init(&ctx_hamsi1);
		sph_hamsi512(&ctx_hamsi1, input, 32);
		sph_hamsi512_close(&ctx_hamsi1, hash);
		break; }
	default:{
		sph_fugue512_context	ctx_fugue1;
		sph_fugue512_init(&ctx_fugue1);
		sph_fugue512(&ctx_fugue1, input, 32);
		sph_fugue512_close(&ctx_fugue1, hash);
		break; }
	}
}
/* suspiciously similar to ScanHash* from bitcoin */
bool scanhash_cryptopp(struct thr_info * const thr, struct work * const work,
	        uint32_t max_nonce, uint32_t *last_nonce,
		uint32_t n)
{
	// const uint8_t *midstate = work->midstate;
	const uint8_t *data = work->data;
	// uint8_t hash1[0x40];
	// memcpy(hash1, hash1_init, sizeof(hash1));
	uint8_t * const hash = work->hash;
	//uint16_t * const hash3 = work->hash;
	uint32_t *const hash32 = (uint32_t *) hash;
	uint32_t *const nonce = (uint32_t *)(data + 76);

	//uint32_t *nonce_w = (uint32_t *)(data +76);
	//data += 64;

	uint8_t sDest[100],temp[500];//,out[32];
	//Hex2Str((unsigned char *)work->data,sDest,76);
	//applog(LOG_DEBUG,"%s",sDest);


	//Hex2Str((unsigned char *)work->data,sDest,80);
	//applog(LOG_DEBUG,"cpu:%d %s",thr->id,sDest);
	//uint8_t data2[80]; 
	//memcpy(data2,data,80);
											//Hex2Str(data,sDest,128);
											//applog(LOG_DEBUG,"cpu:%d 1%s",thr->id,sDest);
	//Hex2Str(data2,sDest,76);
	//applog(LOG_DEBUG,"%s",sDest);

	swap32yes(temp,data,80 / 4);
	//  for(int i=0;i<76;++i)
	//  	temp[i]=data[8*(i/4)+3-i];
		//Hex2Str((unsigned char *)temp,sDest,76);
		//applog(LOG_DEBUG,"计算hash前 %s",sDest);	
	// Midstate and data are stored in little endian
	//LOCAL_swap32le(unsigned char, midstate, 32/4)
	//LOCAL_swap32le(unsigned char, data, 64/4)


	uint8_t base[32],output1[32],output2[32];
	// uint32_t n2=8212953;
	// unsigned char tmmm[]="04000000FF649D499DC996E62B8E50620D996D17A95B99AC59E4C3FBD968FF23CE040000BA3BA0A6656B0EA892D66F982AD14F70707627FCE2DA3B51E2AAEC25A041A3B782F1F959F0FF0F1E";
	// unsigned char temps[80];
	// Hex2byte(tmmm,temps,152);
	// SHA256_CTX ctx1;
	// SHA256Initialize(&ctx1);
	// SHA256Update(&ctx1,(BYTE*)temps,76);
	// SHA256Update(&ctx1,(BYTE*)&n2,4);
	// SHA256Finalize(&ctx1,(BYTE*)hash);
	 //Hex2Str((unsigned char *)hash,sDest,32);
	// applog(LOG_DEBUG,"%s",sDest);

	while (1) {
		*nonce = n;
		
		SHA256_CTX ctx;
		SHA256Initialize(&ctx);
		SHA256Update(&ctx,(BYTE*)temp,76);
		SHA256Update(&ctx,(BYTE*)&n,4);
		SHA256Finalize(&ctx,(BYTE*)base);
		int id0=(((uint16_t *)&base)[0])%13;
		int id1=(((uint16_t *)&base)[1])%13;
		compute(id0,base,output1);
		compute(id1,output1,output2);
		scrypt_1024_1_1_256(output2,sizeof(output2),(uint8_t*)hash);
		//Hex2Str((unsigned char *)temp,sDest,32);
		//applog(LOG_DEBUG,"%s",sDest);

		//runhash(hash1, data, midstate);
		//runhash(hash, hash1, sha256_init_state);

		SHA256Reset(&ctx);
		if (unlikely(hash32[7] <0x0000001f))
		{
			//Hex2Str((unsigned char *)work->data,sDest,76);
			//applog(LOG_DEBUG,"%s",sDest);
			//sleep(0);
			//applog(LOG_DEBUG,"%p",n);
			Hex2Str((uint8_t *)&n,sDest,0);
			//applog(LOG_DEBUG,"%p",n);
			//applog(LOG_DEBUG,"n %s %d",sDest,n);
			//Hex2Str((unsigned char *)work->data,sDest,80);
			//applog(LOG_DEBUG,"source %s",sDest);
			//Hex2Str((unsigned char *)hash,sDest,32);
			//applog(LOG_DEBUG,"found:%s\nnonce %d",sDest,n);
			
											//Hex2Str(data,sDest,128);
											//applog(LOG_DEBUG,"cpu:%d %s",thr->id,sDest);
			//for(int i=0;i<32;i++)
			//temp[i]=hash[-i+8*(i/4)+3];
			//Hex2Str((unsigned char *)temp,sDest,32);

			//applog(LOG_DEBUG,"nonce %d",n);


			//unsigned char * hashas=work->hash;
			// Hex2Str(base,sDest,32);
			// applog(LOG_DEBUG,"base %s",sDest);
			// Hex2Str(output1,sDest,32);
			// applog(LOG_DEBUG,"output1 %s",sDest);
			// Hex2Str(output2,sDest,32);
			// applog(LOG_DEBUG,"output2 %s",sDest);
			// Hex2Str((unsigned char *)hash,sDest,32);
			// applog(LOG_DEBUG,"hash %s",sDest);


			*nonce = htole32(n);
			*last_nonce = n;
			//for(int i=0;i<32;i++)
			//work->hash[i]=temp[i];
			// Hex2Str((unsigned char *)work->data,sDest,80);
			// applog(LOG_DEBUG,"result %s",sDest);


			// Hex2Str((unsigned char *)work->hash,sDest,32);
			// applog(LOG_DEBUG,"wrokhash %s  address ：%d",sDest,work->hash);

			return true;
		}

		if ((n >= max_nonce) || thr->work_restart) {
			*nonce = htole32(n);
			*last_nonce = n;
			//applog(LOG_DEBUG,"none");
			return false;
		}

		n++;
	}
}

#if defined(WANT_CRYPTOPP_ASM32)

#define CRYPTOPP_FASTCALL
#define CRYPTOPP_BOOL_X86 1
#define CRYPTOPP_BOOL_X64 0
#define CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE 0

#ifdef CRYPTOPP_GENERATE_X64_MASM
	#define AS1(x) x*newline*
	#define AS2(x, y) x, y*newline*
	#define AS3(x, y, z) x, y, z*newline*
	#define ASS(x, y, a, b, c, d) x, y, a*64+b*16+c*4+d*newline*
	#define ASL(x) label##x:*newline*
	#define ASJ(x, y, z) x label##y*newline*
	#define ASC(x, y) x label##y*newline*
	#define AS_HEX(y) 0##y##h
#elif defined(_MSC_VER) || defined(__BORLANDC__)
	#define CRYPTOPP_MS_STYLE_INLINE_ASSEMBLY
	#define AS1(x) __asm {x}
	#define AS2(x, y) __asm {x, y}
	#define AS3(x, y, z) __asm {x, y, z}
	#define ASS(x, y, a, b, c, d) __asm {x, y, (a)*64+(b)*16+(c)*4+(d)}
	#define ASL(x) __asm {label##x:}
	#define ASJ(x, y, z) __asm {x label##y}
	#define ASC(x, y) __asm {x label##y}
	#define CRYPTOPP_NAKED __declspec(naked)
	#define AS_HEX(y) 0x##y
#else
	#define CRYPTOPP_GNU_STYLE_INLINE_ASSEMBLY
	// define these in two steps to allow arguments to be expanded
	#define GNU_AS1(x) #x ";"
	#define GNU_AS2(x, y) #x ", " #y ";"
	#define GNU_AS3(x, y, z) #x ", " #y ", " #z ";"
	#define GNU_ASL(x) "\n" #x ":"
	#define GNU_ASJ(x, y, z) #x " " #y #z ";"
	#define AS1(x) GNU_AS1(x)
	#define AS2(x, y) GNU_AS2(x, y)
	#define AS3(x, y, z) GNU_AS3(x, y, z)
	#define ASS(x, y, a, b, c, d) #x ", " #y ", " #a "*64+" #b "*16+" #c "*4+" #d ";"
	#define ASL(x) GNU_ASL(x)
	#define ASJ(x, y, z) GNU_ASJ(x, y, z)
	#define ASC(x, y) #x " " #y ";"
	#define CRYPTOPP_NAKED
	#define AS_HEX(y) 0x##y
#endif

#define IF0(y)
#define IF1(y) y

#ifdef CRYPTOPP_GENERATE_X64_MASM
#define ASM_MOD(x, y) ((x) MOD (y))
#define XMMWORD_PTR XMMWORD PTR
#else
// GNU assembler doesn't seem to have mod operator
#define ASM_MOD(x, y) ((x)-((x)/(y))*(y))
// GAS 2.15 doesn't support XMMWORD PTR. it seems necessary only for MASM
#define XMMWORD_PTR
#endif

#if CRYPTOPP_BOOL_X86
	#define AS_REG_1 ecx
	#define AS_REG_2 edx
	#define AS_REG_3 esi
	#define AS_REG_4 edi
	#define AS_REG_5 eax
	#define AS_REG_6 ebx
	#define AS_REG_7 ebp
	#define AS_REG_1d ecx
	#define AS_REG_2d edx
	#define AS_REG_3d esi
	#define AS_REG_4d edi
	#define AS_REG_5d eax
	#define AS_REG_6d ebx
	#define AS_REG_7d ebp
	#define WORD_SZ 4
	#define WORD_REG(x)	e##x
	#define WORD_PTR DWORD PTR
	#define AS_PUSH_IF86(x) AS1(push e##x)
	#define AS_POP_IF86(x) AS1(pop e##x)
	#define AS_JCXZ jecxz
#elif CRYPTOPP_BOOL_X64
	#ifdef CRYPTOPP_GENERATE_X64_MASM
		#define AS_REG_1 rcx
		#define AS_REG_2 rdx
		#define AS_REG_3 r8
		#define AS_REG_4 r9
		#define AS_REG_5 rax
		#define AS_REG_6 r10
		#define AS_REG_7 r11
		#define AS_REG_1d ecx
		#define AS_REG_2d edx
		#define AS_REG_3d r8d
		#define AS_REG_4d r9d
		#define AS_REG_5d eax
		#define AS_REG_6d r10d
		#define AS_REG_7d r11d
	#else
		#define AS_REG_1 rdi
		#define AS_REG_2 rsi
		#define AS_REG_3 rdx
		#define AS_REG_4 rcx
		#define AS_REG_5 r8
		#define AS_REG_6 r9
		#define AS_REG_7 r10
		#define AS_REG_1d edi
		#define AS_REG_2d esi
		#define AS_REG_3d edx
		#define AS_REG_4d ecx
		#define AS_REG_5d r8d
		#define AS_REG_6d r9d
		#define AS_REG_7d r10d
	#endif
	#define WORD_SZ 8
	#define WORD_REG(x)	r##x
	#define WORD_PTR QWORD PTR
	#define AS_PUSH_IF86(x)
	#define AS_POP_IF86(x)
	#define AS_JCXZ jrcxz
#endif

static void CRYPTOPP_FASTCALL X86_SHA256_HashBlocks(word32 *state, const word32 *data, size_t len
#if defined(_MSC_VER) && (_MSC_VER == 1200)
	, ...	// VC60 workaround: prevent VC 6 from inlining this function
#endif
	)
{
#if defined(_MSC_VER) && (_MSC_VER == 1200)
	AS2(mov ecx, [state])
	AS2(mov edx, [data])
#endif

	#define LOCALS_SIZE	8*4 + 16*4 + 4*WORD_SZ
	#define H(i)		[BASE+ASM_MOD(1024+7-(i),8)*4]
	#define G(i)		H(i+1)
	#define F(i)		H(i+2)
	#define E(i)		H(i+3)
	#define D(i)		H(i+4)
	#define C(i)		H(i+5)
	#define B(i)		H(i+6)
	#define A(i)		H(i+7)
	#define Wt(i)		BASE+8*4+ASM_MOD(1024+15-(i),16)*4
	#define Wt_2(i)		Wt((i)-2)
	#define Wt_15(i)	Wt((i)-15)
	#define Wt_7(i)		Wt((i)-7)
	#define K_END		[BASE+8*4+16*4+0*WORD_SZ]
	#define STATE_SAVE	[BASE+8*4+16*4+1*WORD_SZ]
	#define DATA_SAVE	[BASE+8*4+16*4+2*WORD_SZ]
	#define DATA_END	[BASE+8*4+16*4+3*WORD_SZ]
	#define Kt(i)		WORD_REG(si)+(i)*4
#if CRYPTOPP_BOOL_X86
	#define BASE		esp+4
#elif defined(__GNUC__)
	#define BASE		r8
#else
	#define BASE		rsp
#endif

#define RA0(i, edx, edi)		\
	AS2(	add edx, [Kt(i)]	)\
	AS2(	add edx, [Wt(i)]	)\
	AS2(	add edx, H(i)		)\

#define RA1(i, edx, edi)

#define RB0(i, edx, edi)

#define RB1(i, edx, edi)	\
	AS2(	mov AS_REG_7d, [Wt_2(i)]	)\
	AS2(	mov edi, [Wt_15(i)])\
	AS2(	mov ebx, AS_REG_7d	)\
	AS2(	shr AS_REG_7d, 10		)\
	AS2(	ror ebx, 17		)\
	AS2(	xor AS_REG_7d, ebx	)\
	AS2(	ror ebx, 2		)\
	AS2(	xor ebx, AS_REG_7d	)/* s1(W_t-2) */\
	AS2(	add ebx, [Wt_7(i)])\
	AS2(	mov AS_REG_7d, edi	)\
	AS2(	shr AS_REG_7d, 3		)\
	AS2(	ror edi, 7		)\
	AS2(	add ebx, [Wt(i)])/* s1(W_t-2) + W_t-7 + W_t-16 */\
	AS2(	xor AS_REG_7d, edi	)\
	AS2(	add edx, [Kt(i)])\
	AS2(	ror edi, 11		)\
	AS2(	add edx, H(i)	)\
	AS2(	xor AS_REG_7d, edi	)/* s0(W_t-15) */\
	AS2(	add AS_REG_7d, ebx	)/* W_t = s1(W_t-2) + W_t-7 + s0(W_t-15) W_t-16*/\
	AS2(	mov [Wt(i)], AS_REG_7d)\
	AS2(	add edx, AS_REG_7d	)\

#define ROUND(i, r, eax, ecx, edi, edx)\
	/* in: edi = E	*/\
	/* unused: eax, ecx, temp: ebx, AS_REG_7d, out: edx = T1 */\
	AS2(	mov edx, F(i)	)\
	AS2(	xor edx, G(i)	)\
	AS2(	and edx, edi	)\
	AS2(	xor edx, G(i)	)/* Ch(E,F,G) = (G^(E&(F^G))) */\
	AS2(	mov AS_REG_7d, edi	)\
	AS2(	ror edi, 6		)\
	AS2(	ror AS_REG_7d, 25		)\
	RA##r(i, edx, edi		)/* H + Wt + Kt + Ch(E,F,G) */\
	AS2(	xor AS_REG_7d, edi	)\
	AS2(	ror edi, 5		)\
	AS2(	xor AS_REG_7d, edi	)/* S1(E) */\
	AS2(	add edx, AS_REG_7d	)/* T1 = S1(E) + Ch(E,F,G) + H + Wt + Kt */\
	RB##r(i, edx, edi		)/* H + Wt + Kt + Ch(E,F,G) */\
	/* in: ecx = A, eax = B^C, edx = T1 */\
	/* unused: edx, temp: ebx, AS_REG_7d, out: eax = A, ecx = B^C, edx = E */\
	AS2(	mov ebx, ecx	)\
	AS2(	xor ecx, B(i)	)/* A^B */\
	AS2(	and eax, ecx	)\
	AS2(	xor eax, B(i)	)/* Maj(A,B,C) = B^((A^B)&(B^C) */\
	AS2(	mov AS_REG_7d, ebx	)\
	AS2(	ror ebx, 2		)\
	AS2(	add eax, edx	)/* T1 + Maj(A,B,C) */\
	AS2(	add edx, D(i)	)\
	AS2(	mov D(i), edx	)\
	AS2(	ror AS_REG_7d, 22		)\
	AS2(	xor AS_REG_7d, ebx	)\
	AS2(	ror ebx, 11		)\
	AS2(	xor AS_REG_7d, ebx	)\
	AS2(	add eax, AS_REG_7d	)/* T1 + S0(A) + Maj(A,B,C) */\
	AS2(	mov H(i), eax	)\

#define SWAP_COPY(i)		\
	AS2(	mov		WORD_REG(bx), [WORD_REG(dx)+i*WORD_SZ])\
	AS1(	bswap	WORD_REG(bx))\
	AS2(	mov		[Wt(i*(1+CRYPTOPP_BOOL_X64)+CRYPTOPP_BOOL_X64)], WORD_REG(bx))

#if defined(__GNUC__)
	#if CRYPTOPP_BOOL_X64
		FixedSizeAlignedSecBlock<byte, LOCALS_SIZE> workspace;
	#endif
	__asm__ __volatile__
	(
	#if CRYPTOPP_BOOL_X64
		"lea %4, %%r8;"
	#endif
	".intel_syntax noprefix;"
#elif defined(CRYPTOPP_GENERATE_X64_MASM)
		ALIGN   8
	X86_SHA256_HashBlocks	PROC FRAME
		rex_push_reg rsi
		push_reg rdi
		push_reg rbx
		push_reg rbp
		alloc_stack(LOCALS_SIZE+8)
		.endprolog
		mov rdi, r8
		lea rsi, [?SHA256_K@CryptoPP@@3QBIB + 48*4]
#endif

#if CRYPTOPP_BOOL_X86
	#ifndef __GNUC__
		AS2(	mov		edi, [len])
		AS2(	lea		WORD_REG(si), [SHA256_K+48*4])
	#endif
	#if !defined(_MSC_VER) || (_MSC_VER < 1400)
		AS_PUSH_IF86(bx)
	#endif

	AS_PUSH_IF86(bp)
	AS2(	mov		ebx, esp)
	AS2(	and		esp, -16)
	AS2(	sub		WORD_REG(sp), LOCALS_SIZE)
	AS_PUSH_IF86(bx)
#endif
	AS2(	mov		STATE_SAVE, WORD_REG(cx))
	AS2(	mov		DATA_SAVE, WORD_REG(dx))
	AS2(	lea		WORD_REG(ax), [WORD_REG(di) + WORD_REG(dx)])
	AS2(	mov		DATA_END, WORD_REG(ax))
	AS2(	mov		K_END, WORD_REG(si))

#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
#if CRYPTOPP_BOOL_X86
	AS2(	test	edi, 1)
	ASJ(	jnz,	2, f)
	AS1(	dec		DWORD PTR K_END)
#endif
	AS2(	movdqa	xmm0, XMMWORD_PTR [WORD_REG(cx)+0*16])
	AS2(	movdqa	xmm1, XMMWORD_PTR [WORD_REG(cx)+1*16])
#endif

#if CRYPTOPP_BOOL_X86
#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
	ASJ(	jmp,	0, f)
#endif
	ASL(2)	// non-SSE2
	AS2(	mov		esi, ecx)
	AS2(	lea		edi, A(0))
	AS2(	mov		ecx, 8)
	AS1(	rep movsd)
	AS2(	mov		esi, K_END)
	ASJ(	jmp,	3, f)
#endif

#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
	ASL(0)
	AS2(	movdqa	E(0), xmm1)
	AS2(	movdqa	A(0), xmm0)
#endif
#if CRYPTOPP_BOOL_X86
	ASL(3)
#endif
	AS2(	sub		WORD_REG(si), 48*4)
	SWAP_COPY(0)	SWAP_COPY(1)	SWAP_COPY(2)	SWAP_COPY(3)
	SWAP_COPY(4)	SWAP_COPY(5)	SWAP_COPY(6)	SWAP_COPY(7)
#if CRYPTOPP_BOOL_X86
	SWAP_COPY(8)	SWAP_COPY(9)	SWAP_COPY(10)	SWAP_COPY(11)
	SWAP_COPY(12)	SWAP_COPY(13)	SWAP_COPY(14)	SWAP_COPY(15)
#endif
	AS2(	mov		edi, E(0))	// E
	AS2(	mov		eax, B(0))	// B
	AS2(	xor		eax, C(0))	// B^C
	AS2(	mov		ecx, A(0))	// A

	ROUND(0, 0, eax, ecx, edi, edx)
	ROUND(1, 0, ecx, eax, edx, edi)
	ROUND(2, 0, eax, ecx, edi, edx)
	ROUND(3, 0, ecx, eax, edx, edi)
	ROUND(4, 0, eax, ecx, edi, edx)
	ROUND(5, 0, ecx, eax, edx, edi)
	ROUND(6, 0, eax, ecx, edi, edx)
	ROUND(7, 0, ecx, eax, edx, edi)
	ROUND(8, 0, eax, ecx, edi, edx)
	ROUND(9, 0, ecx, eax, edx, edi)
	ROUND(10, 0, eax, ecx, edi, edx)
	ROUND(11, 0, ecx, eax, edx, edi)
	ROUND(12, 0, eax, ecx, edi, edx)
	ROUND(13, 0, ecx, eax, edx, edi)
	ROUND(14, 0, eax, ecx, edi, edx)
	ROUND(15, 0, ecx, eax, edx, edi)

	ASL(1)
	AS2(add WORD_REG(si), 4*16)
	ROUND(0, 1, eax, ecx, edi, edx)
	ROUND(1, 1, ecx, eax, edx, edi)
	ROUND(2, 1, eax, ecx, edi, edx)
	ROUND(3, 1, ecx, eax, edx, edi)
	ROUND(4, 1, eax, ecx, edi, edx)
	ROUND(5, 1, ecx, eax, edx, edi)
	ROUND(6, 1, eax, ecx, edi, edx)
	ROUND(7, 1, ecx, eax, edx, edi)
	ROUND(8, 1, eax, ecx, edi, edx)
	ROUND(9, 1, ecx, eax, edx, edi)
	ROUND(10, 1, eax, ecx, edi, edx)
	ROUND(11, 1, ecx, eax, edx, edi)
	ROUND(12, 1, eax, ecx, edi, edx)
	ROUND(13, 1, ecx, eax, edx, edi)
	ROUND(14, 1, eax, ecx, edi, edx)
	ROUND(15, 1, ecx, eax, edx, edi)
	AS2(	cmp		WORD_REG(si), K_END)
	ASJ(	jb,		1, b)

	AS2(	mov		WORD_REG(dx), DATA_SAVE)
	AS2(	add		WORD_REG(dx), 64)
	AS2(	mov		AS_REG_7, STATE_SAVE)
	AS2(	mov		DATA_SAVE, WORD_REG(dx))

#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
#if CRYPTOPP_BOOL_X86
	AS2(	test	DWORD PTR K_END, 1)
	ASJ(	jz,		4, f)
#endif
	AS2(	movdqa	xmm1, XMMWORD_PTR [AS_REG_7+1*16])
	AS2(	movdqa	xmm0, XMMWORD_PTR [AS_REG_7+0*16])
	AS2(	paddd	xmm1, E(0))
	AS2(	paddd	xmm0, A(0))
	AS2(	movdqa	[AS_REG_7+1*16], xmm1)
	AS2(	movdqa	[AS_REG_7+0*16], xmm0)
	AS2(	cmp		WORD_REG(dx), DATA_END)
	ASJ(	jb,		0, b)
#endif

#if CRYPTOPP_BOOL_X86
#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
	ASJ(	jmp,	5, f)
	ASL(4)	// non-SSE2
#endif
	AS2(	add		[AS_REG_7+0*4], ecx)	// A
	AS2(	add		[AS_REG_7+4*4], edi)	// E
	AS2(	mov		eax, B(0))
	AS2(	mov		ebx, C(0))
	AS2(	mov		ecx, D(0))
	AS2(	add		[AS_REG_7+1*4], eax)
	AS2(	add		[AS_REG_7+2*4], ebx)
	AS2(	add		[AS_REG_7+3*4], ecx)
	AS2(	mov		eax, F(0))
	AS2(	mov		ebx, G(0))
	AS2(	mov		ecx, H(0))
	AS2(	add		[AS_REG_7+5*4], eax)
	AS2(	add		[AS_REG_7+6*4], ebx)
	AS2(	add		[AS_REG_7+7*4], ecx)
	AS2(	mov		ecx, AS_REG_7d)
	AS2(	cmp		WORD_REG(dx), DATA_END)
	ASJ(	jb,		2, b)
#if CRYPTOPP_BOOL_SSE2_ASM_AVAILABLE
	ASL(5)
#endif
#endif

	AS_POP_IF86(sp)
	AS_POP_IF86(bp)
	#if !defined(_MSC_VER) || (_MSC_VER < 1400)
		AS_POP_IF86(bx)
	#endif

#ifdef CRYPTOPP_GENERATE_X64_MASM
	add		rsp, LOCALS_SIZE+8
	pop		rbp
	pop		rbx
	pop		rdi
	pop		rsi
	ret
	X86_SHA256_HashBlocks ENDP
#endif

#ifdef __GNUC__
	".att_syntax prefix;"
	:
	: "c" (state), "d" (data), "S" (SHA256_K+48), "D" (len)
	#if CRYPTOPP_BOOL_X64
		, "m" (workspace[0])
	#endif
	: "memory", "cc", "%eax"
	#if CRYPTOPP_BOOL_X64
		, "%rbx", "%r8", "%r10"
	#endif
	);
#endif
}

static void SHA256_Transform32(word32 *state, const word32 *data)
{
	word32 W[16];

	swap32yes(W, data, 16);

	X86_SHA256_HashBlocks(state, W, 16 * 4);
}

static void runhash32(void *state, const void *input, const void *init)
{
	memcpy(state, init, 32);
	SHA256_Transform32(state, input);
}

/* suspiciously similar to ScanHash* from bitcoin */
bool scanhash_asm32(struct thr_info * const thr, struct work * const work,
	        uint32_t max_nonce, uint32_t *last_nonce,
		uint32_t n)
{
	const uint8_t * const midstate = work->midstate;
	uint8_t *data = work->data;
	uint8_t hash1[0x40];
	memcpy(hash1, hash1_init, sizeof(hash1));
	uint8_t * const hash = work->hash;
	
	uint32_t *hash32 = (uint32_t *) hash;
	uint32_t *nonce = (uint32_t *)(data + 76);

	data += 64;

	while (1) {
		*nonce = n;
		SHA256_CTX ctx;
		// runhash32(hash1, data, midstate);
		// runhash32(hash, hash1, sha256_init_state);

		if (unlikely(hash32[7] == 0))
		{
			*last_nonce = n;
			return true;
		}

		if ((n >= max_nonce) || thr->work_restart) {
			*last_nonce = n;
			return false;
		}

		++n;
	}
}

#endif	// #if defined(WANT_CRYPTOPP_ASM32)

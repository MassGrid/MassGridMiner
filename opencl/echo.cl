// kernel-interface: echo JumpHash
#include "util_hash.cl"
#ifdef GOFFSET
#endif
#define T32   SPH_T32
#define C32   SPH_C32
#if SPH_64
#define C64   SPH_C64
#endif

#define AES_BIG_ENDIAN   0
#include "aes_helper.cl"

#define ECHO_DECL_STATE_BIG   \
	sph_u64 W00, W01, W10, W11, W20, W21, W30, W31, W40, W41, W50, W51, W60, W61, W70, W71, W80, W81, W90, W91, WA0, WA1, WB0, WB1, WC0, WC1, WD0, WD1, WE0, WE1, WF0, WF1;

#define AES_2ROUNDS(XX, XY)   do { \
		sph_u32 X0 = (sph_u32)(XX); \
		sph_u32 X1 = (sph_u32)(XX >> 32); \
		sph_u32 X2 = (sph_u32)(XY); \
		sph_u32 X3 = (sph_u32)(XY >> 32); \
		sph_u32 Y0, Y1, Y2, Y3; \
		AES_ROUND_LE(X0, X1, X2, X3, K0, K1, K2, K3, Y0, Y1, Y2, Y3); \
		AES_ROUND_NOKEY_LE(Y0, Y1, Y2, Y3, X0, X1, X2, X3); \
		XX = (sph_u64)X0 | ((sph_u64)X1 << 32); \
		XY = (sph_u64)X2 | ((sph_u64)X3 << 32); \
		if ((K0 = T32(K0 + 1)) == 0) { \
			if ((K1 = T32(K1 + 1)) == 0) \
				if ((K2 = T32(K2 + 1)) == 0) \
					K3 = T32(K3 + 1); \
		} \
	} while (0)

#define BIG_SUB_WORDS   do { \
		AES_2ROUNDS(W00, W01); \
		AES_2ROUNDS(W10, W11); \
		AES_2ROUNDS(W20, W21); \
		AES_2ROUNDS(W30, W31); \
		AES_2ROUNDS(W40, W41); \
		AES_2ROUNDS(W50, W51); \
		AES_2ROUNDS(W60, W61); \
		AES_2ROUNDS(W70, W71); \
		AES_2ROUNDS(W80, W81); \
		AES_2ROUNDS(W90, W91); \
		AES_2ROUNDS(WA0, WA1); \
		AES_2ROUNDS(WB0, WB1); \
		AES_2ROUNDS(WC0, WC1); \
		AES_2ROUNDS(WD0, WD1); \
		AES_2ROUNDS(WE0, WE1); \
		AES_2ROUNDS(WF0, WF1); \
	} while (0)

#define SHIFT_ROW1(a, b, c, d)   do { \
		sph_u64 tmp; \
		tmp = W ## a ## 0; \
		W ## a ## 0 = W ## b ## 0; \
		W ## b ## 0 = W ## c ## 0; \
		W ## c ## 0 = W ## d ## 0; \
		W ## d ## 0 = tmp; \
		tmp = W ## a ## 1; \
		W ## a ## 1 = W ## b ## 1; \
		W ## b ## 1 = W ## c ## 1; \
		W ## c ## 1 = W ## d ## 1; \
		W ## d ## 1 = tmp; \
	} while (0)

#define SHIFT_ROW2(a, b, c, d)   do { \
		sph_u64 tmp; \
		tmp = W ## a ## 0; \
		W ## a ## 0 = W ## c ## 0; \
		W ## c ## 0 = tmp; \
		tmp = W ## b ## 0; \
		W ## b ## 0 = W ## d ## 0; \
		W ## d ## 0 = tmp; \
		tmp = W ## a ## 1; \
		W ## a ## 1 = W ## c ## 1; \
		W ## c ## 1 = tmp; \
		tmp = W ## b ## 1; \
		W ## b ## 1 = W ## d ## 1; \
		W ## d ## 1 = tmp; \
	} while (0)

#define SHIFT_ROW3(a, b, c, d)   SHIFT_ROW1(d, c, b, a)

#define BIG_SHIFT_ROWS   do { \
		SHIFT_ROW1(1, 5, 9, D); \
		SHIFT_ROW2(2, 6, A, E); \
		SHIFT_ROW3(3, 7, B, F); \
	} while (0)

#define MIX_COLUMN1(ia, ib, ic, id, n)   do { \
		sph_u64 a = W ## ia ## n; \
		sph_u64 b = W ## ib ## n; \
		sph_u64 c = W ## ic ## n; \
		sph_u64 d = W ## id ## n; \
		sph_u64 ab = a ^ b; \
		sph_u64 bc = b ^ c; \
		sph_u64 cd = c ^ d; \
		sph_u64 abx = ((ab & C64(0x8080808080808080)) >> 7) * 27U \
			^ ((ab & C64(0x7F7F7F7F7F7F7F7F)) << 1); \
		sph_u64 bcx = ((bc & C64(0x8080808080808080)) >> 7) * 27U \
			^ ((bc & C64(0x7F7F7F7F7F7F7F7F)) << 1); \
		sph_u64 cdx = ((cd & C64(0x8080808080808080)) >> 7) * 27U \
			^ ((cd & C64(0x7F7F7F7F7F7F7F7F)) << 1); \
		W ## ia ## n = abx ^ bc ^ d; \
		W ## ib ## n = bcx ^ a ^ cd; \
		W ## ic ## n = cdx ^ ab ^ d; \
		W ## id ## n = abx ^ bcx ^ cdx ^ ab ^ c; \
	} while (0)

#define MIX_COLUMN(a, b, c, d)   do { \
		MIX_COLUMN1(a, b, c, d, 0); \
		MIX_COLUMN1(a, b, c, d, 1); \
	} while (0)

#define BIG_MIX_COLUMNS   do { \
		MIX_COLUMN(0, 1, 2, 3); \
		MIX_COLUMN(4, 5, 6, 7); \
		MIX_COLUMN(8, 9, A, B); \
		MIX_COLUMN(C, D, E, F); \
	} while (0)

#define BIG_ROUND   do { \
		BIG_SUB_WORDS; \
		BIG_SHIFT_ROWS; \
		BIG_MIX_COLUMNS; \
	} while (0)

#define ECHO_COMPRESS_BIG(sc)   do { \
		sph_u32 K0 = sc->C0; \
		sph_u32 K1 = sc->C1; \
		sph_u32 K2 = sc->C2; \
		sph_u32 K3 = sc->C3; \
		unsigned u; \
		INPUT_BLOCK_BIG(sc); \
		for (u = 0; u < 10; u ++) { \
			BIG_ROUND; \
		} \
		ECHO_FINAL_BIG; \
	} while (0)


void echo(hash_t* hash)
{


	__local sph_u32 AES0[256], AES1[256], AES2[256], AES3[256];

	int init = get_local_id(0);
	int step = get_local_size(0);

	for (int i = 0; i < 256; i++)
	{
		AES0[i] = AES0_C[i];
		AES1[i] = AES1_C[i];
		AES2[i] = AES2_C[i];
		AES3[i] = AES3_C[i];
	}

	barrier(CLK_LOCAL_MEM_FENCE);



	// echo
	sph_u64 W00, W01, W10, W11, W20, W21, W30, W31, W40, W41, W50, W51, W60, W61, W70, W71, W80, W81, W90, W91, WA0, WA1, WB0, WB1, WC0, WC1, WD0, WD1, WE0, WE1, WF0, WF1;
	sph_u64 Vb00, Vb01, Vb10, Vb11, Vb20, Vb21, Vb30, Vb31, Vb40, Vb41, Vb50, Vb51, Vb60, Vb61, Vb70, Vb71;
	Vb00 = Vb10 = Vb20 = Vb30 = Vb40 = Vb50 = Vb60 = Vb70 = 512UL;
	Vb01 = Vb11 = Vb21 = Vb31 = Vb41 = Vb51 = Vb61 = Vb71 = 0;

	sph_u32 K0 = 512;
	sph_u32 K1 = 0;
	sph_u32 K2 = 0;
	sph_u32 K3 = 0;

	W00 = Vb00;
	W01 = Vb01;
	W10 = Vb10;
	W11 = Vb11;
	W20 = Vb20;
	W21 = Vb21;
	W30 = Vb30;
	W31 = Vb31;
	W40 = Vb40;
	W41 = Vb41;
	W50 = Vb50;
	W51 = Vb51;
	W60 = Vb60;
	W61 = Vb61;
	W70 = Vb70;
	W71 = Vb71;
	W80 = hash->h8[0];
	W81 = hash->h8[1];
	W90 = hash->h8[2];
	W91 = hash->h8[3];
	WA0 = hash->h8[4];
	WA1 = hash->h8[5];
	WB0 = hash->h8[6];
	WB1 = hash->h8[7];
	WC0 = 0x80;
	WC1 = 0;
	WD0 = 0;
	WD1 = 0;
	WE0 = 0;
	WE1 = 0x200000000000000;
	WF0 = 0x200;
	WF1 = 0;

	for (unsigned u = 0; u < 10; u++) {
		BIG_ROUND;
	}

	hash->h8[0] = hash->h8[0] ^ Vb00 ^ W00 ^ W80;
	hash->h8[1] = hash->h8[1] ^ Vb01 ^ W01 ^ W81;
	hash->h8[2] = hash->h8[2] ^ Vb10 ^ W10 ^ W90;
	hash->h8[3] = hash->h8[3] ^ Vb11 ^ W11 ^ W91;
	hash->h8[4] = hash->h8[4] ^ Vb20 ^ W20 ^ WA0;
	hash->h8[5] = hash->h8[5] ^ Vb21 ^ W21 ^ WA1;
	hash->h8[6] = hash->h8[6] ^ Vb30 ^ W30 ^ WB0;
	hash->h8[7] = hash->h8[7] ^ Vb31 ^ W31 ^ WB1;

	barrier(CLK_GLOBAL_MEM_FENCE);
}


__kernel void scanHash_pre(__global char* input, __global char* output, const uint nonceStart)
{
	uint gid = get_global_id(0);
	//todo : assemble hash data inside kernel with nonce
	hash_t hashdata;
	for (int i = 0; i < 64; i++) {
		hashdata.h1[i] = input[i];
	}
	hashdata.h4[14] = hashdata.h4[14] ^ hashdata.h4[15];
	hashdata.h4[15] = gid + nonceStart;

	echo(&hashdata);
	for (int i = 0; i < 64; i++) {
		output[64*gid+i] = hashdata.h1[i];
	}
	barrier(CLK_GLOBAL_MEM_FENCE);
}
__kernel void scanHash_post(__global char* input, __global char* output,__global uint* goodNonce, const ulong target)
{
	uint gid = get_global_id(0);
	
	//todo : assemble hash data inside kernel with nonce
	hash_t hashdata;
	for (int i = 0; i < 64; i++) {
		hashdata.h1[i] = input[64*gid+i];
	}
	echo(&hashdata);

	ulong outcome = hashdata.h8[3];
	bool result = (outcome <= target);

	if (result) {
		//printf("gid %d hit target!\n", gid);
		goodNonce[0] = gid;
		for (int i = 0; i < 64; i++) {
		output[i] = hashdata.h1[i];
	}
		//[atomic_inc(output + 0xFF)] = SWAP4(gid);
	}
	barrier(CLK_GLOBAL_MEM_FENCE);
}
__kernel void scanHash_check_pre(__global char* input, __global char* output, const uint nonce)
{
	uint gid = get_global_id(0);
	//todo : assemble hash data inside kernel with nonce
	hash_t hashdata;
	for (int i = 0; i < 64; i++) {
		hashdata.h1[i] = input[i];
	}
	hashdata.h4[14] = hashdata.h4[14] ^ hashdata.h4[15];
	hashdata.h4[15] = nonce;

	echo(&hashdata);
	for (int i = 0; i < 64; i++) {
		output[64*gid+i] = hashdata.h1[i];
	}
	barrier(CLK_GLOBAL_MEM_FENCE);
}

__kernel void hashTest(__global char* in, __global char* out)
{
	hash_t input;
	for (int i = 0; i < 64; i++) {
		input.h1[i] = in[i];
	}

	echo(&input);

	for (int i = 0; i < 64; i++) {
		out[i] = input.h1[i];
	}
}


__kernel void helloworld(__global char* in, __global char* out)
{
	int num = get_global_id(0);
	out[num] = in[num] + 1;
}

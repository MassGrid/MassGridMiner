#include "SHA256Digest.h"

/* SHA-256 Constants
 * Represent the first 32 bits of the fractional parts of the
 * cube roots of the first 64 prime numbers
 */
static const WORD k[] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

/* SHA-256 Initial hash values
 * The first 32 bits of the fractional parts of the square roots
 * of the first eight prime numbers
 */
static const WORD h[] = { 0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19 };

/* SHA-256 Functions
 * Using macro:
 */
#define ROTR(a,b) ((a >> b) | (a << (32-b)))
#define CH(x,y,z) ((x & y) ^ (~x & z))
#define MAJ(x,y,z) ((x & y) ^ (x & z) ^ (y & z))
#define EP0(x) (ROTR(x,2) ^ ROTR(x,13) ^ ROTR(x,22))
#define EP1(x) (ROTR(x,6) ^ ROTR(x,11) ^ ROTR(x,25))
#define SIG0(x) (ROTR(x,7) ^ ROTR(x,18) ^ (x >> 3))
#define SIG1(x) (ROTR(x,17) ^ ROTR(x,19) ^ (x >> 10))


static void processChunk(SHA256_CTX *ctx) {
	WORD a, b, c, d, e, f, g, h, w[64], t1, t2;
	
	// copy chunk into the first 16 words of the message schedule array
	for (int i=0, j=0; i<16; i++, j+=4)
		w[i] = (ctx->data[j] << 24) | (ctx->data[j+1] << 16) | (ctx->data[j+2] << 8) | ctx->data[j+3];
	
    // expand the first 16 words into the remaining 48 words of the message schedule array
	for (int i=16; i<64; i++)
		w[i] = w[i-16] + SIG0(w[i-15]) + w[i-7] + SIG1(w[i-2]);

	// initialize working variables:
	a = ctx->hash[0];
	b = ctx->hash[1];
	c = ctx->hash[2];
	d = ctx->hash[3];
	e = ctx->hash[4];
	f = ctx->hash[5];
	g = ctx->hash[6];
	h = ctx->hash[7];
	
	for (int i=0; i<64; i++) {
		t1 = h + EP1(e) + CH(e, f, g) + k[i] + w[i];
		t2 = EP0(a) + MAJ(a, b, c);
		
		h = g;
		g = f;
		f = e;
		e = d + t1;
		d = c;
		c = b;
		b = a;
		a = t1 + t2;
	}

	ctx->hash[0] += a;
	ctx->hash[1] += b;
	ctx->hash[2] += c;
	ctx->hash[3] += d;
	ctx->hash[4] += e;
	ctx->hash[5] += f;
	ctx->hash[6] += g;
	ctx->hash[7] += h;
}

void SHA256Initialize(SHA256_CTX *ctx) {
	memcpy(ctx->hash, h, sizeof h);
	ctx->dataLength = 0;
	ctx->totalLength = 0;
}

void SHA256Update(SHA256_CTX *ctx, const BYTE *in, size_t inLen) {
	ctx->totalLength += inLen*8;
	for (int i=0; i<inLen; i++){
		ctx->data[ctx->dataLength] = in[i];
		ctx->dataLength++;
		if (ctx->dataLength == CHUNK_SIZE) {
			processChunk(ctx);
			ctx->dataLength = 0;
		}
	}
}
void SHA256Reset(SHA256_CTX *ctx)
{
	SHA256Initialize(ctx);

}
void SHA256Finalize(SHA256_CTX *ctx, BYTE *out) {
	int padLength = (ctx->dataLength < 56) ? (64 - ctx->dataLength) : (120 - ctx->dataLength);
	
	//Create the padding:
	BYTE padding[2*CHUNK_SIZE];
	memset(padding, 0, sizeof padding);
	padding[0] = 0x80;
	
	//Put the total message length in bits into the padding:
	for (int i=0; i<8; i++)
		padding[padLength-1-i] = ctx->totalLength >> 8*i;
		
	//Process the last chunk(s) with padding
	SHA256Update(ctx, padding, padLength);
	
	//Convert to big endian and put into output
	for (int i=0, j=0; i<8; i++) {
		out[j++] = (ctx->hash[i] >> 24) & 0xff;
		out[j++] = (ctx->hash[i] >> 16) & 0xff;
		out[j++] = (ctx->hash[i] >> 8 ) & 0xff;
		out[j++] = ctx->hash[i] & 0xff;
	}
}

void SHA256ComputeDigest(BYTE *in, size_t inLen, BYTE *out) {
	SHA256_CTX ctx;
	SHA256Initialize(&ctx);
	SHA256Update(&ctx, in, inLen);
	SHA256Finalize(&ctx, out);
}

int SHA256DigestSize() {
	return SHA256_DIGEST_LENGTH;
}
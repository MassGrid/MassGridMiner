#ifndef _W_WHIRLPOOL_H_
#define _W_WHIRLPOOL_H_
#ifdef __cplusplus
extern "C" {
#endif
	void whirlpool_scanHash_pre(unsigned char* input, unsigned  char* output, const unsigned int nonce);
	void whirlpool_scanHash_post(unsigned char* input, unsigned char* output);
#ifdef __cplusplus
}
#endif
#endif

#include <stdlib.h>
#include <stdint.h>
void scrypt_1024_1_1_256(const char *input, size_t inputlen, char *output);
void scrypt_1024_1_1_256_sp_generic(const char *input, size_t inputlen, char *output, char *scratchpad);
/** A hasher class for Scrypt-256. */
//remove CScrypt256 class which before is a cpp file

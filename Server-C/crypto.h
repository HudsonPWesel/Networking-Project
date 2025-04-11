#include <stdio.h>
void sha256_to_hex(const unsigned char *hash, char *output);
size_t calcDecodeLength(const char *b64input);
int Base64Decode(char *b64message, unsigned char **buffer, size_t *length);
int Base64Encode(const unsigned char *buffer, size_t length, char **b64text);
void generate_random_token(char *token, size_t length);

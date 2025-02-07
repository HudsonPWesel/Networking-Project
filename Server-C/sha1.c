#include "sha1.h"
#include <string.h>

void sha1_hash(const char *input, unsigned char output[SHA1_DIGEST_LENGTH]) {
  CC_SHA1(input, (CC_LONG)strlen(input), output);
}

void print_hash(unsigned char hash[SHA1_DIGEST_LENGTH]) {
  for (int i = 0; i < SHA1_DIGEST_LENGTH; i++) {
    printf("%02x", hash[i]);
  }
  printf("\n");
}

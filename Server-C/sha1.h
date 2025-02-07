#ifndef SHA1_UTIL_H
#define SHA1_UTIL_H

#include <stdio.h>
#include <CommonCrypto/CommonDigest.h>

#define SHA1_DIGEST_LENGTH CC_SHA1_DIGEST_LENGTH

// Function to compute SHA1 hash
void sha1_hash(const char *input, unsigned char output[SHA1_DIGEST_LENGTH]);

// Function to print SHA1 hash in hex format
void print_hash(unsigned char hash[SHA1_DIGEST_LENGTH]);

#endif // SHA1_UTIL_H

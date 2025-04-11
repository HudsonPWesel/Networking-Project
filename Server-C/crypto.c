#include <assert.h>
#include <openssl/buffer.h>
#include <openssl/rand.h>
#include <openssl/sha.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void sha256_to_hex(const unsigned char *hash, char *output) {
  for (int i = 0; i < 32; ++i) {
    sprintf(output + (i * 2), "%02x", hash[i]);
  }
  output[64] = '\0';
}

size_t calcDecodeLength(
    const char *b64input) { // Calculates the length of a decoded string
  size_t len = strlen(b64input), padding = 0;

  if (b64input[len - 1] == '=' &&
      b64input[len - 2] == '=') // last two chars are =
    padding = 2;
  else if (b64input[len - 1] == '=') // last char is =
    padding = 1;

  return (len * 3) / 4 - padding;
}

int Base64Decode(char *b64message, unsigned char **buffer,
                 size_t *length) { // Decodes a base64 encoded string
  BIO *bio, *b64;

  int decodeLen = calcDecodeLength(b64message);
  *buffer = (unsigned char *)malloc(decodeLen + 1);
  (*buffer)[decodeLen] = '\0';

  bio = BIO_new_mem_buf(b64message, -1);
  b64 = BIO_new(BIO_f_base64());
  bio = BIO_push(b64, bio);

  BIO_set_flags(bio,
                BIO_FLAGS_BASE64_NO_NL); // Do not use newlines to flush buffer
  *length = BIO_read(bio, *buffer, strlen(b64message));
  assert(*length == decodeLen); // length should equal decodeLen, else something
                                // went horribly wrong
  BIO_free_all(bio);

  return (0); // success
}

int Base64Encode(const unsigned char *buffer, size_t length,
                 char **b64text) { // Encodes a binary safe base 64 string
  BIO *bio, *b64;
  BUF_MEM *bufferPtr;

  b64 = BIO_new(BIO_f_base64());
  bio = BIO_new(BIO_s_mem());
  bio = BIO_push(b64, bio);

  BIO_set_flags(
      bio,
      BIO_FLAGS_BASE64_NO_NL); // Ignore newlines - write everything in one line
  BIO_write(bio, buffer, length);
  BIO_flush(bio);
  BIO_get_mem_ptr(bio, &bufferPtr);
  BIO_set_close(bio, BIO_NOCLOSE);
  BIO_free_all(bio);

  *b64text = (*bufferPtr).data;

  return (0); // success
}

void generate_random_token(char *token, size_t length) {
  const char charset[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
  if (length) {
    for (size_t i = 0; i < length - 1; i++) {
      int key = rand() % (int)(sizeof(charset) - 1);
      token[i] = charset[key];
    }
    token[length - 1] = '\0'; // Null-terminate the token string
  }
}

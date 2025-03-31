#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *base64_encode(const unsigned char *input, size_t length) {
  static const char encode_table[] =
      "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  char *output = malloc((4 * ((length + 2) / 3)) + 1);
  size_t i, j = 0;

  for (i = 0; i < length; i += 3) {
    unsigned int octet_a = i < length ? input[i] : 0;
    unsigned int octet_b = i + 1 < length ? input[i + 1] : 0;
    unsigned int octet_c = i + 2 < length ? input[i + 2] : 0;

    unsigned int triple = (octet_a << 16) + (octet_b << 8) + octet_c;

    output[j++] = encode_table[(triple >> 18) & 0x3F];
    output[j++] = encode_table[(triple >> 12) & 0x3F];
    output[j++] = encode_table[(triple >> 6) & 0x3F];
    output[j++] = encode_table[triple & 0x3F];
  }

  size_t padding = length % 3;
  if (padding) {
    for (i = 0; i < padding; ++i)
      output[j - (i + 1)] = '='; // Add padding
  }

  output[j] = '\0';
  return output;
}

    /***********************************************************
    * Base64 library implementation                            *
    * @author Ahmed Elzoughby                                  *
    * @date July 23, 2017                                      *
    ***********************************************************/

#include "base64.h"


char base46_map[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                     'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                     'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                     'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', '8', '9', '+', '/'};


    char* base64_encode(unsigned char* data, size_t input_length) {
      char* cipher = malloc(input_length * 4 / 3 + 4);
      int i, c = 0;
      int input_index = 0;

      // Process full 3-byte groups
      for(; input_index + 2 < input_length; input_index += 3) {
        cipher[c++] = base46_map[data[input_index] >> 2];
        cipher[c++] = base46_map[((data[input_index] & 0x03) << 4) | (data[input_index + 1] >> 4)];
        cipher[c++] = base46_map[((data[input_index + 1] & 0x0f) << 2) | (data[input_index + 2] >> 6)];
        cipher[c++] = base46_map[data[input_index + 2] & 0x3f];
      }

      // Handle remaining bytes (1 or 2)
      if (input_index < input_length) {
        cipher[c++] = base46_map[data[input_index] >> 2];

        if (input_index + 1 < input_length) {
          cipher[c++] = base46_map[((data[input_index] & 0x03) << 4) | (data[input_index + 1] >> 4)];
          cipher[c++] = base46_map[(data[input_index + 1] & 0x0f) << 2];
          cipher[c++] = '=';
        } else {
          cipher[c++] = base46_map[(data[input_index] & 0x03) << 4];
          cipher[c++] = '=';
          cipher[c++] = '=';
        }
      }

      cipher[c] = '\0';
      return cipher;
    }


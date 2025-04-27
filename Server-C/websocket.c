#include "websocket.h"
#include "auth.h"
#include "crypto.h"
#include "game.h"
#include <arpa/inet.h>
#include <cjson/cJSON.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <openssl/bio.h>
#include <openssl/sha.h>
#include <openssl/ssl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define MASKING_KEY_LENGTH 4
#define MAXLINE 1024
#define KEY_LENGTH 24
#define GUID_LENGTH 36
#define KEY_NAME_LENGTH 19

const char *MAGIC_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

char *base64_encode(const unsigned char *input, int length) {
  BIO *bmem = NULL, *b64 = NULL;
  BUF_MEM *bptr;

  b64 = BIO_new(BIO_f_base64());
  bmem = BIO_new(BIO_s_mem());
  b64 = BIO_push(b64, bmem);
  BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
  BIO_write(b64, input, length);
  BIO_flush(b64);
  BIO_get_mem_ptr(b64, &bptr);

  char *buff = (char *)malloc(bptr->length + 1);
  memcpy(buff, bptr->data, bptr->length);
  buff[bptr->length] = 0;

  BIO_free_all(b64);

  return buff;
}

void send_websocket_message(int client_fd, const char *message) {
  size_t len = strlen(message);
  unsigned char frame[14];
  int idx = 0;

  frame[idx++] = 0x81; // FIN bit set + opcode 0x1 (text frame)

  if (len <= 125) {
    frame[idx++] = (unsigned char)len;
  } else if (len <= 65535) {
    frame[idx++] = 126;
    frame[idx++] = (len >> 8) & 0xFF;
    frame[idx++] = len & 0xFF;
  } else {
    frame[idx++] = 127;
    // For simplicity, assume len fits into 4 bytes (real large files need 8
    // bytes)
    for (int i = 7; i >= 0; --i) {
      frame[idx++] = (i >= 4) ? 0 : (len >> (8 * i)) & 0xFF;
    }
  }

  send(client_fd, frame, idx, 0);
  send(client_fd, message, len, 0);
}
cJSON *websocket_decode(char *buffer, int length, int client_fd) {
  if (length < 2) {
    printf("Invalid frame: too short\n");
    return NULL;
  }

  uint8_t is_fin = (buffer[0] & 0x80) >> 7;
  uint8_t opcode = buffer[0] & 0x0F;
  uint8_t is_masked = (buffer[1] & 0x80) >> 7;
  uint64_t payload_len = buffer[1] & 0x7F;
  int mask_offset = 2;

  if (opcode == 0x8) {
    printf("Received close frame\n");
    close(client_fd);
    return NULL;
  } else if (opcode != 0x1) {
    printf("Unsupported opcode: %d\n", opcode);
    return NULL;
  }

  if (payload_len == 126) {
    if (length < 4)
      return NULL;
    payload_len = (buffer[2] << 8) | buffer[3];
    mask_offset = 4;
  } else if (payload_len == 127) {
    if (length < 10)
      return NULL;
    payload_len = 0;
    for (int i = 2; i < 10; i++)
      payload_len = (payload_len << 8) | buffer[i];
    mask_offset = 10;
  }

  if (is_masked) {
    if (length < mask_offset + 4)
      return NULL;
  }

  int data_offset = mask_offset + 4;
  if (length < data_offset + payload_len) {
    printf("Invalid frame: payload length mismatch\n");
    return NULL;
  }

  char decoded_message[payload_len + 1];
  if (is_masked) {
    char masking_key[4];
    memcpy(masking_key, buffer + mask_offset, 4);
    for (uint64_t i = 0; i < payload_len; i++)
      decoded_message[i] = buffer[data_offset + i] ^ masking_key[i % 4];
  } else {
    memcpy(decoded_message, buffer + data_offset, payload_len);
  }
  decoded_message[payload_len] = '\0';

  printf("Decoded Message: %s\n", decoded_message);
  return cJSON_Parse(decoded_message);
}

void respond_handshake(char *buffer, int client_fd) {
  char *key_start = strstr(buffer, "Sec-WebSocket-Key: ");
  if (!key_start) {
    printf("No WebSocket Key found\n");
    return;
  }

  key_start += strlen("Sec-WebSocket-Key: ");
  char *key_end = strstr(key_start, "\r\n");
  if (!key_end)
    return;

  char client_key[256] = {0};
  strncpy(client_key, key_start, key_end - key_start);

  char accept_key[512];
  snprintf(accept_key, sizeof(accept_key), "%s%s", client_key, MAGIC_GUID);

  unsigned char sha1_result[SHA_DIGEST_LENGTH];
  SHA1((unsigned char *)accept_key, strlen(accept_key), sha1_result);

  char *encoded = base64_encode(sha1_result, SHA_DIGEST_LENGTH);

  char response[1024];
  snprintf(response, sizeof(response),
           "HTTP/1.1 101 Switching Protocols\r\n"
           "Upgrade: websocket\r\n"
           "Connection: Upgrade\r\n"
           "Sec-WebSocket-Accept: %s\r\n\r\n",
           encoded);

  send(client_fd, response, strlen(response), 0);
  free(encoded);
}

void process_new_connection(ServerState *state, fd_set *ready_set) {
  struct sockaddr_in cli_addr;
  socklen_t cli_len = sizeof(cli_addr);

  printf("\nProcessing new Conn\n");

  int temp_fd = accept(state->listenfd, (struct sockaddr *)&cli_addr, &cli_len);
  if (temp_fd < 0) {
    perror("Accept Error");
    return;
  }

  // Make non-blocking
  int flags = fcntl(temp_fd, F_GETFL, 0);
  if (flags == -1)
    flags = 0;
  fcntl(temp_fd, F_SETFL, flags | O_NONBLOCK);

  // Find empty slot
  int slot = 0;
  while (slot < FD_SETSIZE && state->client[slot] >= 0)
    slot++;

  if (slot >= FD_SETSIZE) {
    printf("Too many clients!\n");
    close(temp_fd);
    return;
  }

  FD_SET(temp_fd, &(state->allset));
  state->client[slot] = temp_fd;
  state->handshake_done[slot] = 0;

  if (temp_fd > state->maxfd)
    state->maxfd = temp_fd;
  if (slot > state->maxi)
    state->maxi = slot;

  char client_ip[INET_ADDRSTRLEN];
  inet_ntop(AF_INET, &(cli_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
  printf("New connection from %s:%d (fd: %d, slot: %d)\n", client_ip,
         ntohs(cli_addr.sin_port), temp_fd, slot);
}

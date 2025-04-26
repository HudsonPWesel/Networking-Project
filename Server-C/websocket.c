#include "websocket.h"
#include "auth.h"
#include "crypto.h"
#include "game.h"
#include <arpa/inet.h>
#include <cjson/cJSON.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <openssl/sha.h>
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

void send_websocket_message(int client_fd, const char *message) {
  printf("Sending message to client %d: %s\n", client_fd, message);
  size_t message_len = strlen(message);
  unsigned char frame[10];
  size_t frame_size = 0;

  if (client_fd < 0) {
    printf("Invalid client_fd: %d\n", client_fd);
    return;
  }

  frame[0] = 0x81;

  if (message_len <= 125) {
    frame[1] = message_len;
    frame_size = 2;
  } else if (message_len <= 65535) {
    frame[1] = 126;
    frame[2] = (message_len >> 8) & 0xFF;
    frame[3] = message_len & 0xFF;
    frame_size = 4;
  } else {
    frame[1] = 127;
    printf("Message too long (len: %zu), not sending\n", message_len);
    return; // Message too long
  }

  printf("Frame size: %zu, message length: %zu\n", frame_size, message_len);

  ssize_t sent1 = send(client_fd, frame, frame_size, 0);
  ssize_t sent2 = send(client_fd, message, message_len, 0);

  if (sent1 < 0 || sent2 < 0) {
    perror("Error sending websocket message");
  } else {
    printf("Successfully sent %zd + %zd bytes\n", sent1, sent2);
  }
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
  printf("\n== NEW CONN ==\n");
  char *key_line = strstr(buffer, "Sec-WebSocket-Key:");
  if (!key_line)
    return;

  key_line += strlen("Sec-WebSocket-Key:");
  while (*key_line == ' ')
    key_line++;

  char *key_end = strstr(key_line, "\r\n");
  if (!key_end)
    return;

  char sec_key[128] = {0};
  size_t key_len = key_end - key_line;
  if (key_len >= sizeof(sec_key))
    key_len = sizeof(sec_key) - 1;

  memcpy(sec_key, key_line, key_len);

  char combined[256];
  snprintf(combined, sizeof(combined), "%s258EAFA5-E914-47DA-95CA-C5AB0DC85B11",
           sec_key);

  unsigned char sha1_hash[SHA_DIGEST_LENGTH];
  SHA1((unsigned char *)combined, strlen(combined), sha1_hash);

  char *accept_key = NULL;
  Base64Encode(sha1_hash, SHA_DIGEST_LENGTH, &accept_key);

  char response[512];
  snprintf(response, sizeof(response),
           "HTTP/1.1 101 Switching Protocols\r\n"
           "Upgrade: websocket\r\n"
           "Connection: Upgrade\r\n"
           "Sec-WebSocket-Accept: %s\r\n\r\n",
           accept_key);

  printf("\nRESPONDED TO FD : %s WITH WEBSOCKET-KEY %s", sec_key, accept_key);

  write(client_fd, response, strlen(response));
  free(accept_key);
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

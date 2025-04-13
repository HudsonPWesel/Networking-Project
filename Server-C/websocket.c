#include "websocket.h"
#include "auth.h"
#include "crypto.h"
#include <arpa/inet.h>
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
  size_t message_len = strlen(message);
  unsigned char frame[10];
  size_t frame_size = 0;

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
    return;
  }
  send(client_fd, frame, frame_size, 0);
  send(client_fd, message, message_len, 0);
}
cJSON *websocket_decode(char *buffer, int length, int client_fd) {
  if (length < 2) {
    printf("Invalid frame: too short\n");
    return NULL;
  }

  uint8_t is_fin = (buffer[0] & 0x80) >> 7;
  uint8_t opcode = buffer[0] & 0x0F;
  uint8_t is_masked = (buffer[1] & 0x80) >> 7;
  uint8_t payload_len = buffer[1] & 0x7F;
  int mask_offset = 2;

  // if (opcode == 0x8) {
  //   printf("Received close frame\n");
  //   close(client_fd);
  //   return NULL;
  // } else if (opcode != 0x1) {
  //   printf("Received unsupported frame (opcode: %d)\n", opcode);
  //   return NULL;
  // }

  if (payload_len == 126) {
    if (length < 4) {
      printf("Invalid frame\n");
      return NULL;
    }
    payload_len = (buffer[2] << 8) | buffer[3];
    mask_offset = 4;
  } else if (payload_len == 127) {
    if (length < 10) {
      printf("Invalid frame\n");
      return NULL;
    }
    payload_len = 0;
    for (int i = 2; i < 10; i++) {
      payload_len = (payload_len << 8) | buffer[i];
    }
    mask_offset = 10;
  }

  if (is_masked) {
    if (length < mask_offset + MASKING_KEY_LENGTH) {
      printf("Invalid frame: too short for masking key\n");
      return NULL;
    }
  }
  char masking_key[MASKING_KEY_LENGTH];
  memcpy(masking_key, buffer + mask_offset, MASKING_KEY_LENGTH);
  int data_offset = mask_offset + MASKING_KEY_LENGTH;

  if (length < data_offset + payload_len) {
    printf("Invalid frame: payload length mismatch\n");
    return NULL;
  }

  char decoded_message[payload_len + 1];
  for (int i = 0; i < payload_len; i++)
    decoded_message[i] =
        buffer[data_offset + i] ^ masking_key[i % MASKING_KEY_LENGTH];

  decoded_message[payload_len] = '\0';

  printf("Decoded Message: %s\n", decoded_message);

  memset(buffer, 0, strlen(buffer));
  memcpy(buffer, decoded_message, payload_len + 1);

  cJSON *json = cJSON_Parse(buffer);
  if (json == NULL) {
    printf("Error parsing JSON\n");
    return NULL;
  }

  return json;
}

// FIXME :

void respond_handshake(char *buffer, int client_fd) {
  printf("GOT NEW CONN\n");
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

  write(client_fd, response, strlen(response));
  free(accept_key);
}

void process_new_connection(ServerState *state) {
  struct sockaddr_in cli_addr;
  socklen_t cli_len = sizeof(cli_addr);
  char buffer[MAXLINE];

  if (FD_ISSET(state->listenfd, &(state->rset))) {
    int temp_fd =
        accept(state->listenfd, (struct sockaddr *)&cli_addr, &cli_len);
    if (temp_fd < 0) {
      perror("Accept Error\n");
      return;
    }

    int n = recv(temp_fd, buffer, sizeof(buffer) - 1, MSG_PEEK);
    if (n <= 0) {
      close(temp_fd);
      return;
    }
    buffer[n] = '\0';

    if (!strstr(buffer, "Sec-WebSocket-Key")) {
      close(temp_fd);
      return;
    }

    read(temp_fd, buffer, sizeof(buffer) - 1);
    buffer[n] = '\0';
    printf("Received handshake:\n%s\n", buffer);

    char *key_start = strstr(buffer, "Sec-WebSocket-Key");
    if (key_start)
      respond_handshake(key_start, temp_fd);

    int i = 0;
    while (state->client[i] >= 0 && i < FD_SETSIZE)
      i++;

    if (i >= FD_SETSIZE) {
      printf("Too many clients!\n");
      close(temp_fd);
      return;
    }

    FD_SET(temp_fd, &(state->allset));
    state->client[i] = temp_fd;

    if (temp_fd > state->maxfd)
      state->maxfd = temp_fd;

    if (i > state->maxi)
      state->maxi = i;

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(cli_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    printf("New WebSocket client connected from %s:%d (fd: %d, slot: %d)\n",
           client_ip, ntohs(cli_addr.sin_port), state->client[i], i);

    for (int i = 0; i <= state->maxi; i++) {
      if (state->client[i] >= 0) {
        printf("Client %d active (fd: %d)\n", i, state->client[i]);
      }
    }
  }
}

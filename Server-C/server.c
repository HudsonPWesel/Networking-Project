#include "base64.h"
#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 9999
#define LISTEN_BACKLOG 50
#define KEY_NAME_LENGTH 19
#define KEY_LENGTH 24
#define GUID_LENGTH 36
#define MAXLINE 1024
#define MASKING_KEY_LENGTH 4

#if defined(_WIN32)
#define GETSOCKETERRNO() (WSAGetLastError())
#else
#define GETSOCKETERRNO() (errno)
#endif

typedef struct ServerState {
  int maxi; // index into client[] array
  int maxfd;
  int client[FD_SETSIZE];
  fd_set allset;
  fd_set rset;
  int listenfd;

} ServerState;

void respond_handshake(char *key_start, int client_fd);
void initialize_state(ServerState *state, int listenfd);
void process_new_connection(ServerState *state);

void websocket_decode(char *buffer, int length) {

  uint8_t is_fin = (buffer[0] & 0x80) >> 7;
  uint8_t opcode = buffer[0] & 0x0F;
  uint8_t is_masked = (buffer[1] & 0x80) >> 7;
  uint8_t payload_len = buffer[1] & 0x7F;
  char masking_key[4];
  int counter = 0;

  memcpy(masking_key, buffer + 2, MASKING_KEY_LENGTH);

  for (int i = 6; i < 14; i++) {
    printf("%c ", (buffer[i] ^ (masking_key[counter % 4])));
    counter++;
  }

  // int mask_offset = 1;

  // printf("\nis_fin : %u\nopcode : %u\nis_masked : %d\npayload_len : %d\n\n",
  //        is_fin, opcode, is_masked, payload_len);

  // if (payload_len == 126) {
  //   payload_len = (buffer[2] << 7) | buffer[3];
  //   mask_offset = 4;
  // } else if (payload_len == 127) {
  //   for (int i = 1; i < 10; i++)
  //     payload_len += buffer[i];
  //   mask_offset = 10;
  // }
}

void initialize_state(ServerState *state, int listenfd) {
  state->listenfd = listenfd;
  state->maxfd = listenfd;

  state->maxi = -1;

  for (int i = 0; i < FD_SETSIZE; i++)
    state->client[i] = -1;

  FD_ZERO(&(state->allset));
  FD_SET(listenfd, &(state->allset));
}

void process_new_connection(ServerState *state) {
  printf("Proessing Conn...\n");

  struct sockaddr_in cli_addr;
  socklen_t cli_len = sizeof(cli_addr);
  int i = 0;
  char buffer[MAXLINE];
  int connfd;

  if (FD_ISSET(state->listenfd, &(state->rset))) {
    if ((connfd = accept(state->listenfd, (struct sockaddr *)&cli_addr,
                         &cli_len)) < 0) {
      perror("Accept Error\n");
      return;
    }

    while (state->client[i] >= 0 && i < FD_SETSIZE)
      i++;

    if (i > FD_SETSIZE) {
      printf("Too many clients!\n");
      return;
    }

    read(connfd, buffer, sizeof(buffer));
    printf("%s", buffer);
    char *key_start = strstr(buffer, "Sec-WebSocket-Key");
    printf("%s\n", key_start);

    if (key_start)
      respond_handshake(key_start, connfd);

    FD_SET(connfd, &(state->allset));
    state->client[i] = connfd;

    if (connfd > state->maxfd)
      state->maxfd = connfd;

    if (i > state->maxi) {
      state->maxi = i;
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(cli_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    printf("New client () connected from %s:%d (fd: %d, slot: %d)\n", client_ip,
           ntohs(cli_addr.sin_port), state->client[i], i);
  }
}

void process_client_data(ServerState *state) {
  int current_fd, read_bytes;

  char buffer[MAXLINE];
  bzero(&buffer, sizeof(buffer));

  for (int i = 0; i < state->maxi + 1; i++) {
    if ((current_fd = state->client[i]) < 0)
      continue;
    if (FD_ISSET(current_fd, &state->rset)) {
      if ((read_bytes = read(state->client[i], buffer, sizeof(buffer))) == 0) {
        close(current_fd);
        FD_CLR(current_fd, &state->rset);
        state->client[i] = -1;
      } else {
        websocket_decode(buffer, read_bytes);
        buffer[read_bytes - 1] = '\0';
        printf("%s", buffer);
      }
    }
  }
}

void respond_handshake(char *key_start, int client_fd) {
  printf("RUNNING");

  char web_socket_key[KEY_LENGTH + GUID_LENGTH + 1];

  char const socket_guid[37] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

  char buffer[MAXLINE];

  memcpy(web_socket_key, key_start + KEY_NAME_LENGTH, KEY_LENGTH);

  web_socket_key[KEY_LENGTH] = '\0';
  strcat(web_socket_key, socket_guid);

  unsigned char sha1_digest[SHA_DIGEST_LENGTH];
  SHA1((unsigned char *)web_socket_key, strlen(web_socket_key), sha1_digest);

  char *encoded_key = base64_encode((char *)sha1_digest);

  printf("Encoded Key: %s\nSize: %lu\n", encoded_key, strlen(encoded_key));

  char response[256];
  bzero(&buffer, sizeof(buffer));
  bzero(&response, sizeof(response));

  sprintf(response,
          "HTTP/1.1 101 Switching Protocols\r\n"
          "Upgrade: websocket\r\n"
          "Connection: Upgrade\r\n"
          "Sec-WebSocket-Accept: %s\r\n"
          "Access-Control-Allow-Origin: *\r\n" // Allow requests from any origin
          "Access-Control-Allow-Methods: GET, POST\r\n"
          "Access-Control-Allow-Headers: content-type\r\n\r\n",
          encoded_key);

  write(client_fd, response, strlen(response));
}

int main(int argc, char const *argv[]) {
  printf("Running Server...\n");

  int server_fd;

  char buffer[MAXLINE];
  bzero(&buffer, sizeof(buffer));

  struct sockaddr_in server_addr;

  bzero(&server_addr, sizeof(server_addr));

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Socket Error");
    exit(2);
  }

  int yes = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&yes,
                 sizeof(yes)) < 0)
    fprintf(stderr, "setsockopt() failed. Error: %d\n", GETSOCKETERRNO());

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    perror("Bind Error");
    exit(3);
  }

  if (listen(server_fd, LISTEN_BACKLOG) == -1) {
    perror("Listen Error");
    exit(4);
  }

  ServerState state;
  initialize_state(&state, server_fd);

  for (;;) {
    state.rset = state.allset;
    int nready = select(state.maxfd + 1, &(state.rset), NULL, NULL, NULL);
    printf("N Ready %d\n", nready);

    if (nready < 0) {
      perror("None Ready");
      continue;
    }
    process_new_connection(&state);
    process_client_data(&state);
  }

  return 0;
}

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/evp.h>
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

#include <assert.h>

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
  if (length < 2) {
    printf("Invalid frame: too short\n");
    return;
  }

  uint8_t is_fin = (buffer[0] & 0x80) >> 7;
  uint8_t opcode = buffer[0] & 0x0F;
  uint8_t is_masked = (buffer[1] & 0x80) >> 7;
  uint8_t payload_len = buffer[1] & 0x7F;
  int mask_offset = 2;

  if (payload_len == 126) {
    if (length < 4) {
      printf("Invalid frame: too short for extended payload length\n");
      return;
    }
    payload_len = (buffer[2] << 8) | buffer[3];
    mask_offset = 4;
  } else if (payload_len == 127) {
    if (length < 10) {
      printf("Invalid frame: too short for extended payload length\n");
      return;
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
      return;
    }
  }

  char masking_key[MASKING_KEY_LENGTH];
  memcpy(masking_key, buffer + mask_offset, MASKING_KEY_LENGTH);
  int data_offset = mask_offset + MASKING_KEY_LENGTH;

  if (length < data_offset + payload_len) {
    printf("Invalid frame: payload length mismatch\n");
    return;
  }

  printf("Decoded Message: ");
  for (int i = 0; i < payload_len; i++) {
    printf("%c", buffer[data_offset + i] ^ masking_key[i % MASKING_KEY_LENGTH]);
  }
  printf("\n");

  printf("Frame Info:\n");
  printf("is_fin: %u\n", is_fin);
  printf("opcode: %u\n", opcode);
  printf("is_masked: %u\n", is_masked);
  printf("payload_len: %d\n", payload_len);
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
  struct sockaddr_in cli_addr;
  socklen_t cli_len = sizeof(cli_addr);
  int connfd;
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

    connfd = temp_fd;
    read(connfd, buffer, sizeof(buffer) - 1);
    buffer[n] = '\0';

    char *key_start = strstr(buffer, "Sec-WebSocket-Key");
    if (key_start) {
      respond_handshake(key_start, connfd);
    }

    int i = 0;
    while (state->client[i] >= 0 && i < FD_SETSIZE)
      i++;

    if (i >= FD_SETSIZE) {
      printf("Too many clients!\n");
      close(connfd);
      return;
    }

    FD_SET(connfd, &(state->allset));
    state->client[i] = connfd;

    if (connfd > state->maxfd)
      state->maxfd = connfd;

    if (i > state->maxi)
      state->maxi = i;

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(cli_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    printf("New WebSocket client connected from %s:%d (fd: %d, slot: %d)\n",
           client_ip, ntohs(cli_addr.sin_port), state->client[i], i);
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

  char web_socket_key[KEY_LENGTH + GUID_LENGTH + 1];

  char const socket_guid[37] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

  char buffer[MAXLINE];

  memcpy(web_socket_key, key_start + KEY_NAME_LENGTH, KEY_LENGTH);

  web_socket_key[KEY_LENGTH] = '\0';
  strcat(web_socket_key, socket_guid);

  char sha1_digest[SHA_DIGEST_LENGTH];
  SHA1(web_socket_key, strlen(web_socket_key), sha1_digest);

  char *encoded_key;

  Base64Encode((const unsigned char *)sha1_digest, strlen(sha1_digest),
               &encoded_key);

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

    if (nready < 0) {
      perror("None Ready");
      continue;
    }
    process_new_connection(&state);
    process_client_data(&state);
  }

  return 0;
}

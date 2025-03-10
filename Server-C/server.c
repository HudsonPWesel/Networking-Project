#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "base64.h"
#include "sha1.h"

#define PORT 4444
#define LISTEN_BACKLOG 50
#define KEY_NAME_LENGTH 19
#define KEY_LENGTH 24
#define GUID_LENGTH 36
#define MAXLINE 1024

typedef struct ServerState {
  int maxi;  // index into client[] array
  int maxfd;
  int client[FD_SETSIZE];
  fd_set allset;
  fd_set rset;
  int listenfd;

} ServerState;

void respond_handshake(char *key_start, int client_fd); 
void initialize_state(ServerState *state, int listenfd);
void process_new_connection(ServerState *state);


void initialize_state(ServerState *state, int listenfd){

  state->listenfd = listenfd;
  state->maxfd = listenfd; 

  state->maxi = -1;

  for (int i = 0; i < FD_SETSIZE; i++) 
    state->client[i] = -1;

  FD_ZERO(&(state->allset));
  FD_SET(listenfd, &(state->allset));

}

void process_new_connection(ServerState *state){

  struct sockaddr_in cli_addr;
  socklen_t cli_len = sizeof(cli_addr);
  int i = 0;
  char buffer[MAXLINE];
  int connfd;

  if (FD_ISSET(state->listenfd, &(state->rset))) {
    if((connfd = accept(state->listenfd, (struct sockaddr *)&cli_addr, &cli_len)) < 0){
      perror("Accept Error\n");
      return;
    }

    while (state->client[i] >= 0 && i < FD_SETSIZE) i++;

    if (i > FD_SETSIZE ){
      printf("Too many clients!\n");
      return;
    }

    read(connfd, buffer, sizeof(buffer));
    printf("%s", buffer);
    char *key_start = strstr(buffer, "Sec-WebSocket-Key");
    if (key_start) respond_handshake(key_start, connfd);

    FD_SET(connfd, &(state->allset));
    state->client[i] = connfd;

    if (connfd > state->maxfd) 
      state->maxfd = connfd;

    if (i > state->maxi) {
      state->maxi = i;
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(cli_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
    printf("New client () connected from %s:%d (fd: %d, slot: %d)\n", 
           client_ip, ntohs(cli_addr.sin_port), state->client[i], i);
  }
}

void process_client_data(ServerState *state){
  int current_fd, read_bytes;

  char buffer[MAXLINE];
  bzero(&buffer,sizeof(buffer));

  for (int i = 0;i < state->maxi + 1;i++) {
    if((current_fd = state->client[i]) < 0) continue;
    if(FD_ISSET(current_fd, &state->rset) ) {
      printf("Printing Data...");
      if ((read_bytes = read(state->client[i], buffer,sizeof(buffer))) == 0){
        close(current_fd);
        FD_CLR(current_fd, &state->rset);
        state->client[i] = -1;
      }else {
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

  unsigned char sha1_digest[SHA1_DIGEST_LENGTH];
  sha1_hash(web_socket_key, sha1_digest);

  char *encoded_key = base64_encode(sha1_digest, SHA1_DIGEST_LENGTH);

  printf("Encoded Key: %s\nSize: %lu\n", encoded_key, strlen(encoded_key));

  char response[256];
  bzero(&buffer, sizeof(buffer));
  bzero(&response, sizeof(response));

  sprintf(response,
          "HTTP/1.1 101 Switching Protocols\r\n"
          "Upgrade: websocket\r\n"
          "Connection: Upgrade\r\n"
          "Sec-WebSocket-Accept: %s\r\n\r\n",
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

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
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

    if (nready < 0){
      perror("None Ready");
      continue;
    }
    process_new_connection(&state);
    process_client_data(&state);
  }

  return 0;
}



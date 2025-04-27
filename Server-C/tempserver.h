#ifndef SERVER_H
#define SERVER_H
#include <cjson/cJSON.h>
#include <errno.h>
#include <stdio.h>
#include <strings.h>
#include <sys/socket.h>
#include <unistd.h>
#define PORT 9999
#define MAXLINE 1024
#define GETSOCKETERRNO() (errno)
#define LISTEN_BACKLOG 50

typedef struct {
  int listenfd;
  int maxfd;
  int maxi; // index into client array
  fd_set allset;
  fd_set rset;
  int client[FD_SETSIZE];
  int handshake_done[FD_SETSIZE]; // NEW
} ServerState;

int Socket();
void init_serverstate(ServerState *state, int server_fd);
void process_client_data(ServerState *state, fd_set *ready_set);
void process_client_messages(ServerState *state, int client_idx,
                             int current_fd);

#endif

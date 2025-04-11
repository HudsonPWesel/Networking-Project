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

typedef struct ServerState {
  int maxi; // index into client[] array
  int maxfd;
  int client[FD_SETSIZE];
  fd_set allset;
  fd_set rset;
  int listenfd;

} ServerState;

int Socket();
void init_serverstate(ServerState *state, int server_fd);
ServerState init_ServerState(int server_fd);

void process_client_data(ServerState *state);

#endif

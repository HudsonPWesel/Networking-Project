// EchoServer.c
// To compiler: gcc -Wall EchoServer.c -o ES
// To run: ./ES
#include <arpa/inet.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#define MAXLINE 1024
#define LISTENQ 1024
#define SERV_PORT 49049
// #define FD_SETSIZE 10 or take the default of 1024
#define bool int
#define true 1
#define false 0
#define SA struct sockaddr
void str_echo(int sockfd);
void sig_child(int signc);

typedef struct ServerState {
    int maxi;  // index into client[] array
    int maxfd;
    int client[FD_SETSIZE];
    fd_set allset;
    fd_set rset;
    int listenfd;

} ServerState;

int Socket();
void process_clients(int listenfd);
void process_new_connection(ServerState *state);

int Socket() {
    struct sockaddr_in serv_addr;
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd < 0) {
        perror("socket error");
        exit(1);
    }

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_PORT);

    if (bind(listenfd, (SA *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("bind error");
        exit(3);
    }

    if (listen(listenfd, LISTENQ) < 0) {
        perror("listen error");
        exit(4);
    }

    return listenfd;
}

void process_client_data(ServerState *state){
  int sockfd, n;
  char buff [MAXLINE];

  for (int i = 0; i <= state->maxi; i++) {
    if ((sockfd = state->client[i]) < 0) continue;
    if (FD_ISSET(sockfd, &(state->rset))) {
      if ((n = read(sockfd, buff, MAXLINE)) == 0) {
        close(sockfd);
        FD_CLR(sockfd, &(state->allset));
        state->client[i] = -1;
      } else {
        buff[n - 1] = '\0';
        n--;
        printf("READ %d: %s\n", sockfd, buff);
        write(sockfd, buff, n);
        printf("WRITE %d: %s\n", sockfd, buff);
      }
    }
  }
}

void process_new_connection(ServerState *state){

  struct sockaddr_in cli_addr;
  socklen_t cli_len = sizeof(cli_addr);
  int i = 0;

  if (FD_ISSET(state->listenfd, &(state->rset))) {
    int connfd = accept(state->listenfd, (SA *)&cli_addr, &cli_len);
    if (connfd < 0) {
        fprintf(stderr, "accept error\n");
        return;
    }

    while (state->client[i] >= 0 && i < FD_SETSIZE) i++;

    if (i < FD_SETSIZE) {
      state->client[i] = connfd;
    } else {
      printf("Too many clients!\n");
      exit(1);
    }
    printf(" Client: %d fd: %d\n", i, connfd);

    FD_SET(connfd, &(state->allset));
    if (connfd > state->maxfd) 
            state->maxfd = connfd;
        
    if (i > state->maxi) 
      state->maxi = i;
    
  }
}

void initialize_state(ServerState *state, int listenfd){

  state->listenfd = listenfd;
  state->maxfd = listenfd; 

  state->maxi = -1;

  for (int i = 0; i < FD_SETSIZE; i++) 
    state->client[i] = -1;

  FD_ZERO(&(state->allset));
  FD_SET(listenfd, &(state->allset));

}

void process_clients(int listenfd){

  ServerState state;
  initialize_state(&state, listenfd);

  for (;;) {
    state.rset = state.allset;
    int nready = select(state.maxfd + 1, &(state.rset), NULL, NULL, NULL);
    if (nready < 0){
      fprintf(stderr, "Select Error");
      continue;
    }

    process_new_connection(&state);
    process_client_data(&state);
  }
}

int main(int argc, char **argv) {
  int listenfd = Socket();

  printf("The number of clients = %d\n", FD_SETSIZE);
  process_clients(listenfd);
  return 0;
}

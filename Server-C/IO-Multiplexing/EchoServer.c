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

Socket();
void process_clients(int listenfd);

int Socket() {
    struct sockaddr_in serv_addr;

    int listenfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    serv_addr.sin_port = htons(SERV_PORT);

    bind(listenfd, (SA *)&serv_addr, sizeof(serv_addr));
    listen(listenfd, LISTENQ);

    return listenfd;
}

void process_clients(int listenfd){

  int  maxi, maxfd, connfd, sockfd;
  fd_set rset, allset;

  maxfd = listenfd;
  maxi = -1;

  int nready, i, client[FD_SETSIZE];
  size_t n;
  socklen_t cli_len;

  struct sockaddr_in cli_addr;
  char buff[MAXLINE];

  for (i = 0; i < FD_SETSIZE; i++) 
    client[i] = -1;

  FD_ZERO(&allset);
  FD_SET(listenfd, &allset);

  for (;;) {
    rset = allset;
    nready = select(maxfd + 1, &rset, NULL, NULL, NULL);
    if (FD_ISSET(listenfd, &rset)) {
      cli_len = sizeof(cli_addr);
      connfd = accept(listenfd, (SA *)&cli_addr, &cli_len);
      i = 0;  // find an unused client to store the socket id
      while (client[i] >= 0 && i < FD_SETSIZE) i++;
      if (i < FD_SETSIZE) {
        client[i] = connfd;
      } else {
        printf("Too many clients!\n");
        exit(1);
      }
      printf(" Client: %d fd: %d\n", i, connfd);
      FD_SET(connfd, &allset);
      if (connfd > maxfd) maxfd = connfd;
      if (i > maxi) maxi = i;
      if (--nready <= 0) continue;  // no more readable descriptors
    }

    for (i = 0; i <= maxi; i++) {
      if ((sockfd = client[i]) < 0) continue;
      if (FD_ISSET(sockfd, &rset)) {
        if ((n = read(sockfd, buff, MAXLINE)) == 0) {
          close(sockfd);
          FD_CLR(sockfd, &allset);
          client[i] = -1;
        } else {
          buff[n - 1] = '\0';
          n--;
          printf("READ %d: %s\n", sockfd, buff);
          write(sockfd, buff, n);
          printf("WRITE %d: %s\n", sockfd, buff);
        }
        if (--nready <= 0) break;
      }
    }
  }
}

int main(int argc, char **argv) {
    int listenfd = Socket();
    
    printf("The number of clients = %d\n", FD_SETSIZE);

    process_clients(listenfd);
}

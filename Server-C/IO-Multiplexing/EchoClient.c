// To compiler: gcc -Wall EchoClient.c -o EC
// To run: ./EC <server IP address>
#include <arpa/inet.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define MAXLINE 1024
#define SERV_PORT 49049
#define bool int
#define true 1
#define false 0
#define SA struct sockaddr

void str_cli(FILE *fp, int sockfd);


void process_stdin_input(int sockfd, FILE * fp, fd_set rset, int *stdineof){
  char sendline [MAXLINE];
  int n;

  if (FD_ISSET(fileno(fp), &rset)) {
    if ((n = read(fileno(fp), sendline, MAXLINE)) == 0) {
      * stdineof = 1;
      shutdown(sockfd, SHUT_WR);  
      FD_CLR(fileno(fp), &rset);
    } else {
      sendline[n] = '\0';  
      write(sockfd, sendline, n);
    }
  }
}
void process_socket_input(int sockfd){
  char recvline[MAXLINE];
  int n;

  if ((n = read(sockfd, recvline, MAXLINE)) == 0) {
    printf("str_cli: server terminated prematurely\n");
    exit(1);
  }
  recvline[n++] = '\n';  
  recvline[n] = '\0';  
  write(fileno(stdout), recvline, n);

}

int Socket(char *server_ip) {
    int sockfd;
    struct sockaddr_in servaddr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("socket error\n");
        exit(2);
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT); /*daytime server - normally 13*/
    if (inet_pton(AF_INET, server_ip, &servaddr.sin_addr) <= 0)
        printf("inet_pton error for %s\n", server_ip);
    if (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) < 0) {
        printf("connect error\n");
        exit(3);
    }
    str_cli(stdin, sockfd);
    exit(0);
}

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Usage: a.out <IPaddress>\n");
        exit(1);
    }
  int client_fd = Socket(argv[1]);

}

int max(int a, int b) { return (a > b) ? a : b; }

void str_cli(FILE *fp, int sockfd) {
    int maxfdp1;
    fd_set rset;
    int n;
    int stdineof = 0;

    FD_ZERO(&rset);
    printf("Client ready\n");

    for (;;) {
      if (stdineof == 0) FD_SET(fileno(fp), &rset);
      FD_SET(sockfd, &rset);
      maxfdp1 = max(fileno(fp), sockfd) + 1;
      select(maxfdp1, &rset, NULL, NULL, NULL);

      if (FD_ISSET(sockfd, &rset)){
        process_socket_input(sockfd);

    }
        process_stdin_input(sockfd, fp, rset, &stdineof);
    }
}

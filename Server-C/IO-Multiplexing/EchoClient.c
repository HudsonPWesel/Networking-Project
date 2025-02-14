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
int main(int argc, char **argv) {
    int sockfd;
    struct sockaddr_in servaddr;
    if (argc != 2) {
        printf("Usage: a.out <IPaddress>\n");
        exit(1);
    }
    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("socket error\n");
        exit(2);
    }
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(SERV_PORT); /*daytime server - normally 13*/
    if (inet_pton(AF_INET, argv[1], &servaddr.sin_addr) <= 0)
        printf("inet_pton error for %s\n", argv[1]);
    if (connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) < 0) {
        printf("connect error\n");
        exit(3);
    }
    str_cli(stdin, sockfd);
    exit(0);
}
int max(int a, int b) { return (a > b) ? a : b; }
void str_cli(FILE *fp, int sockfd) {
    char sendline[MAXLINE];
    char recvline[MAXLINE];
    int maxfdp1;
    int stdineof;
    int n;
    fd_set rset;
    stdineof = 0;
    FD_ZERO(&rset);
    printf("Client ready\n");
    sendline[0] = '\0';
    for (;;) {
        if (stdineof == 0) FD_SET(fileno(fp), &rset);
        FD_SET(sockfd, &rset);
        maxfdp1 = max(fileno(fp), sockfd) + 1;
        select(maxfdp1, &rset, NULL, NULL, NULL);
        if (FD_ISSET(sockfd, &rset)) {
            if ((n = read(sockfd, recvline, MAXLINE)) == 0) {
                if (stdineof == 1)
                    return;  // normal termination
                else {
                    printf("str_cli: server terminated prematurely\n");
                    exit(1);
                }
            }
            recvline[n++] = '\n';  // server does not add on the newline.
            // need that for the output if you use
            // the write system call. See next comment.
            recvline[n] = '\0';  // really don't need this if you use n in
            // the write function. If you want to use
            // strlen, then you need to put put in a
            // NULL char since the read function does not.
            write(fileno(stdout), recvline, n);
        }
        if (FD_ISSET(fileno(fp), &rset)) {
            if ((n = read(fileno(fp), sendline, MAXLINE)) == 0) {
                // probably received an EOF marker on stdin - ^d
                stdineof = 1;
                shutdown(sockfd, SHUT_WR);  // end FIN
                FD_CLR(fileno(fp), &rset);
            } else {
                sendline[n] = '\0';  // really don't need this if you use n in
                // the write function. If you want to use
                // strlen, then you need to put put in a
                // NULL char since the read function does not.
                write(sockfd, sendline, n);
            }
        }
    }
}
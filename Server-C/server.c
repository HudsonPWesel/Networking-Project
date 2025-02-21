#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h> 
#include <netinet/in.h> 
#include <unistd.h> 
#include "sha1.h"
#include "base64.h"

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
    char usernames[MAXLINE][FD_SETSIZE];
    fd_set allset;
    fd_set rset;
    int listenfd;

} ServerState;

int main(int argc, char const *argv[]) {

    char buffer[MAXLINE];
    memset(buffer, 0, sizeof(buffer));

    char const socket_guid [37] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    int server_fd, client_fd;
    socklen_t peer_addr_size;
    struct sockaddr_in server_addr, client_addr;

    memset(&server_addr,0,sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    server_fd = socket(AF_INET,SOCK_STREAM,0);


    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        exit(6);
    }

    if(listen(server_fd, LISTEN_BACKLOG) == -1){
      perror("listen");
      exit(4);
    }

    peer_addr_size = sizeof(client_addr);

    char web_socket_key[KEY_LENGTH + GUID_LENGTH + 1];

    for (;;){

    if((client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &peer_addr_size)) < 0){
      fprintf(stderr, "accept error\n");
    }

    if(client_fd != -1){
      read(client_fd, buffer, sizeof(buffer));
      printf("Full Header %s\n=======================", buffer);

      char *key_start = strstr(buffer, "Sec-WebSocket-Key"); 

      if(key_start){
        memcpy(web_socket_key, strstr(buffer,"Sec-WebSocket-Key")+KEY_NAME_LENGTH,KEY_LENGTH);
        web_socket_key[KEY_LENGTH] = '\0';
        strcat(web_socket_key,socket_guid);

        unsigned char sha1_digest[SHA1_DIGEST_LENGTH];
        sha1_hash(web_socket_key, sha1_digest);

        char* encoded_key = base64_encode(sha1_digest, SHA1_DIGEST_LENGTH);

        printf("Encoded Key: %s\nSize: %lu\n", encoded_key, strlen(encoded_key));

        char response[256];
        memset(response,0,sizeof(response));
        memset(buffer,0,sizeof(buffer));

        sprintf(response,
                "HTTP/1.1 101 Switching Protocols\r\n"
                "Upgrade: websocket\r\n"
                "Connection: Upgrade\r\n"
                "Sec-WebSocket-Accept: %s\r\n\r\n",
                encoded_key);
        
        write(client_fd, response, strlen(response));


      }else{
        printf("Sec-WebSocket-Key not found in request\n");

        exit(EXIT_FAILURE);
      }
      
    }
  }

  return 0;
}


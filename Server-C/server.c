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
#define SHA1_DIGEST_LENGTH 20

int main(int argc, char const *argv[]) {

    char buffer[1024];
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
    char sha1_digest [SHA1_DIGEST_LENGTH + 1];

    for (;;){

      client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &peer_addr_size);

    if(client_fd != -1){
      read(client_fd, buffer, sizeof(buffer));
      printf("Full Header %s\n=======================", buffer);

      char *key_start = strstr(buffer, "Sec-WebSocket-Key"); 

      if(key_start){
        memcpy(web_socket_key, strstr(buffer,"Sec-WebSocket-Key")+KEY_NAME_LENGTH,KEY_LENGTH);
        web_socket_key[KEY_LENGTH] = '\0';
        strcat(web_socket_key,socket_guid);
        SHA1(sha1_digest, web_socket_key, strlen(web_socket_key));

        printf("\n Web Socket Key %s\n=====================\n Size of web_socket_key %lu\n", web_socket_key, sizeof(web_socket_key));

        printf("%s", sha1_digest);

      }else{
        printf("Sec-WebSocket-Key not found in request\n");

        exit(EXIT_FAILURE);
      }


      // == Crafting Sec-WebSocket-Accept == BASE64(SHA-1(KEY + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11")) (this could be bac)
      // Remember each header line ends with \r\n and put an extra \r\n after the last one to indicate the end of the header):
      
    }


  }

  return 0;
}


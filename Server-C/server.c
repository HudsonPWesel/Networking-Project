#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <openssl/sha.h>

#define PORT 4444
#define LISTEN_BACKLOG 50
#define KEY_NAME_LENGTH 19
#define KEY_LENGTH 24
#define GUID_LENGTH 36
#define SHA1_DIGEST_LENGTH 20

int main(int argc, char const *argv[]) {

    char buffer[1024];
    char const SOCKET_GUID [37] = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

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

  // Client multiple connections
  // Multiple games
  //

  // KEY_LENGTH + '\0'

  char web_socket_key[KEY_LENGTH + GUID_LENGTH + 1];
  char sha1_digest [SHA1_DIGEST_LENGTH + 1];

  for (;;){

    client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &peer_addr_size);

    if(client_fd){
      read(client_fd, buffer, sizeof(buffer));
      if((memcpy(web_socket_key, strstr(buffer,"Sec-WebSocket-Key")+KEY_NAME_LENGTH,KEY_LENGTH))){
        strcat(web_socket_key,SOCKET_GUID);
        printf("%s\n", web_socket_key);

        web_socket_key[KEY_LENGTH + GUID_LENGTH] = '\0';
      }

        //printf("Full Header %s\n=======================", buffer);
        //printf("\n Web Socket Key %s\n=====================", web_socket_key);
      

      // Intiate handler respose
      // == Crafting Sec-WebSocket-Accept == BASE64(SHA-1(KEY + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11")) (this could be bac)
      // Remember each header line ends with \r\n and put an extra \r\n after the last one to indicate the end of the header):
      //
    }


  }

  return 0;
}


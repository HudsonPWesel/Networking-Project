// websocket_server.c
#include <openssl/buffer.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <openssl/sha.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define PORT 9999
#define BUF_SIZE 4096

// WebSocket magic GUID
const char *MAGIC_GUID = "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

// Base64 encoding (simple version)
#include <openssl/bio.h>
#include <openssl/evp.h>

char *base64_encode(const unsigned char *input, int length) {
    BIO *bmem = NULL, *b64 = NULL;
    BUF_MEM *bptr;

    b64 = BIO_new(BIO_f_base64());
    bmem = BIO_new(BIO_s_mem());
    b64 = BIO_push(b64, bmem);
    BIO_set_flags(b64, BIO_FLAGS_BASE64_NO_NL);
    BIO_write(b64, input, length);
    BIO_flush(b64);
    BIO_get_mem_ptr(b64, &bptr);

    char *buff = (char *)malloc(bptr->length + 1);
    memcpy(buff, bptr->data, bptr->length);
    buff[bptr->length] = 0;

    BIO_free_all(b64);

    return buff;
}

// Perform WebSocket handshake
void websocket_handshake(int client_fd, const char *request) {
    char *key_start = strstr(request, "Sec-WebSocket-Key: ");
    if (!key_start) {
        printf("No WebSocket Key found\n");
        return;
    }

    key_start += strlen("Sec-WebSocket-Key: ");
    char *key_end = strstr(key_start, "\r\n");
    if (!key_end) return;

    char client_key[256] = {0};
    strncpy(client_key, key_start, key_end - key_start);

    char accept_key[512];
    snprintf(accept_key, sizeof(accept_key), "%s%s", client_key, MAGIC_GUID);

    unsigned char sha1_result[SHA_DIGEST_LENGTH];
    SHA1((unsigned char *)accept_key, strlen(accept_key), sha1_result);

    char *encoded = base64_encode(sha1_result, SHA_DIGEST_LENGTH);

    char response[1024];
    snprintf(response, sizeof(response),
             "HTTP/1.1 101 Switching Protocols\r\n"
             "Upgrade: websocket\r\n"
             "Connection: Upgrade\r\n"
             "Sec-WebSocket-Accept: %s\r\n\r\n",
             encoded);

    send(client_fd, response, strlen(response), 0);
    free(encoded);
}

// Decode a WebSocket frame (just simple text frames)
int websocket_receive(int client_fd, char *buffer, size_t size) {
    unsigned char hdr[2];
    if (recv(client_fd, hdr, 2, 0) <= 0) return -1;

    int payload_len = hdr[1] & 0x7F;
    unsigned char mask[4];

    if (recv(client_fd, mask, 4, 0) <= 0) return -1;

    char payload[BUF_SIZE] = {0};
    if (recv(client_fd, payload, payload_len, 0) <= 0) return -1;

    for (int i = 0; i < payload_len; i++) {
        payload[i] ^= mask[i % 4];
    }

    payload[payload_len] = '\0';
    strncpy(buffer, payload, size - 1);
    return payload_len;
}

// Send simple text frame back
void websocket_send(int client_fd, const char *message) {
    size_t len = strlen(message);
    unsigned char frame[10];
    int idx = 0;
    frame[idx++] = 0x81; // FIN + text frame
    if (len <= 125) {
        frame[idx++] = len;
    }
    send(client_fd, frame, idx, 0);
    send(client_fd, message, len, 0);
}

int main() {
    int server_fd, client_fd, max_fd;
    struct sockaddr_in address;
    fd_set master_set, read_fds;
    char buffer[BUF_SIZE];

    // Create TCP socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 10) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    FD_ZERO(&master_set);
    FD_SET(server_fd, &master_set);
    max_fd = server_fd;

    printf("WebSocket server running on port %d...\n", PORT);

    while (1) {
        read_fds = master_set;

        if (select(max_fd + 1, &read_fds, NULL, NULL, NULL) < 0) {
            perror("select error");
            exit(EXIT_FAILURE);
        }

        for (int fd = 0; fd <= max_fd; fd++) {
            if (FD_ISSET(fd, &read_fds)) {
                if (fd == server_fd) {
                    // New connection
                    socklen_t addrlen = sizeof(address);
                    if ((client_fd = accept(server_fd, (struct sockaddr *)&address, &addrlen)) < 0) {
                        perror("accept");
                        continue;
                    }
                    FD_SET(client_fd, &master_set);
                    if (client_fd > max_fd) max_fd = client_fd;
                    printf("New client connected: %d\n", client_fd);
                } else {
                    int bytes_read = recv(fd, buffer, sizeof(buffer) - 1, MSG_PEEK);
                    if (bytes_read <= 0) {
                        printf("Client disconnected: %d\n", fd);
                        close(fd);
                        FD_CLR(fd, &master_set);
                        continue;
                    }

                    // Peek if it's HTTP handshake
                    buffer[bytes_read] = 0;
                    if (strstr(buffer, "Upgrade: websocket")) {
                        recv(fd, buffer, sizeof(buffer) - 1, 0); // clear data
                        websocket_handshake(fd, buffer);
                    } else {
                        char message[BUF_SIZE];
                        if (websocket_receive(fd, message, sizeof(message)) > 0) {
                            printf("Received: %s\n", message);
                            websocket_send(fd, message); // Echo back
                        }
                    }
                }
            }
        }
    }

    return 0;
}


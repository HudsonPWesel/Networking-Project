#ifndef WEBSOCKET_H
#define WEBSOCKET_H
#include <arpa/inet.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "server.h"
cJSON *websocket_decode(char *buffer, int length, int client_fd);
void respond_handshake(char *key_start, int client_fd);
void send_websocket_message(int client_fd, const char *message);
void process_new_connection(ServerState *state);

#endif

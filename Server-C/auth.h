#ifndef AUTH_H
#define AUTH_H
#include "crypto.h"
#include "database.h"
#include "websocket.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <time.h>
#define TOKEN_LENGTH 32

cJSON *parse_json(char *decoded_message);
void send_session_token(int client_fd, const char *session_token,
                        char *username);
void handle_signup_or_login(cJSON *json, int client_fd);

#endif

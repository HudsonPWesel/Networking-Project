#include "server.h"
#include "auth.h"
#include "game.h"
#include "websocket.h"
#include <string.h>

void init_serverstate(ServerState *state, int server_fd) {
  state->listenfd = server_fd;
  state->maxfd = server_fd;

  state->maxi = -1;

  for (int i = 0; i < FD_SETSIZE; i++)
    state->client[i] = -1;

  FD_ZERO(&(state->allset));
  FD_SET(server_fd, &(state->allset));
}

void process_client_data(ServerState *state) {
  printf("Processing Client Data ...");
  int current_fd, read_bytes;

  char buffer[MAXLINE];
  bzero(&buffer, sizeof(buffer));

  for (int i = 0; i <= state->maxi; i++) {
    if ((current_fd = state->client[i]) < 0)
      continue;

    if (FD_ISSET(current_fd, &state->rset)) {
      read_bytes = read(current_fd, buffer, sizeof(buffer) - 1);
      if (read_bytes == 0) {
        close(current_fd);
        FD_CLR(current_fd, &state->rset);
        state->client[i] = -1;
      } else if (read_bytes < 0) {
        perror("Read error");
        continue;
      } else {
        buffer[read_bytes] = '\0';
        cJSON *json_data = websocket_decode(buffer, read_bytes, current_fd);
        const cJSON *type = NULL;
        if (!json_data)
          return;
        type = cJSON_GetObjectItemCaseSensitive(json_data, "type");
        if (!strcmp(type->valuestring, "login") ||
            !strcmp(type->valuestring, "signup")) {
          handle_signup_or_login(json_data, current_fd);
          add_player_to_queue(json_data, current_fd);
        } else if (!strcmp(type->valuestring, "move")) {
          handle_game_move(json_data, current_fd);
        }
      }
    }
  }
}
int Socket() {
  int server_fd;

  char buffer[MAXLINE];
  bzero(&buffer, sizeof(buffer));

  struct sockaddr_in server_addr;

  bzero(&server_addr, sizeof(server_addr));

  server_addr.sin_family = AF_INET;
  server_addr.sin_port = htons(PORT);
  server_addr.sin_addr.s_addr = INADDR_ANY;

  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    perror("Socket Error");
    exit(2);
  }

  int yes = 1;
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (void *)&yes,
                 sizeof(yes)) < 0)
    fprintf(stderr, "setsockopt() failed. Error: %d\n", GETSOCKETERRNO());

  if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) ==
      -1) {
    perror("Bind Error");
    exit(3);
  }

  if (listen(server_fd, LISTEN_BACKLOG) == -1) {
    perror("Listen Error");
    exit(4);
  }
  return server_fd;
}

ServerState init_ServerState(int server_fd) {
  ServerState state;
  init_serverstate(&state, server_fd);

  return state;
}

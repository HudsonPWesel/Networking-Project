#include "server.h"
#include "auth.h"
#include "game.h"
#include "websocket.h"
#include <string.h>

void init_serverstate(ServerState *state, int server_fd) {
  state->listenfd = server_fd;
  state->maxfd = server_fd;
  state->maxi = -1;
  for (int i = 0; i < FD_SETSIZE; i++) {
    state->client[i] = -1;
    state->handshake_done[i] = -1; // <- meaning "no client yet"
  }

  FD_ZERO(&(state->allset));
  FD_SET(server_fd, &(state->allset));
}

void process_client_data(ServerState *state, fd_set *ready_set) {
  printf("\nProcessing Client Data ...\n");
  int current_fd;

  for (int i = 0; i <= state->maxi; i++) {
    if ((current_fd = state->client[i]) < 0)
      continue;

    if (FD_ISSET(current_fd, ready_set)) {
      process_client_messages(state, i, current_fd);
    }
  }
}
void process_client_messages(ServerState *state, int client_idx,
                             int current_fd) {
  char buffer[MAXLINE];
  int continue_processing = 1;

  while (continue_processing) {
    bzero(buffer, sizeof(buffer));

    // Peek at available data without removing from socket buffer
    int peek_bytes = recv(current_fd, buffer, sizeof(buffer) - 1, MSG_PEEK);

    if (peek_bytes <= 0) {
      printf("Client on fd %d closed connection\n", current_fd);
      close(current_fd);
      FD_CLR(current_fd, &(state->allset));
      state->client[client_idx] = -1;
      state->handshake_done[client_idx] = 0; // Reset handshake status
      return; // Exit the function as client disconnected
    }

    // Handle client based on handshake status
    if (state->handshake_done[client_idx] == 0) {
      // Process WebSocket handshake
      int read_bytes = recv(current_fd, buffer, sizeof(buffer) - 1, 0);
      if (read_bytes > 0) {
        buffer[read_bytes] = '\0';
        respond_handshake(buffer, current_fd);
        state->handshake_done[client_idx] = 1;
        printf("Handshake completed with client %d (fd: %d)\n", client_idx,
               current_fd);
      } else {
        printf("Handshake failed from client %d (fd: %d)\n", client_idx,
               current_fd);
        close(current_fd);
        FD_CLR(current_fd, &(state->allset));
        state->client[client_idx] = -1;
        state->handshake_done[client_idx] = 0;
      }
      return; // Handshake complete, will process frames on next select()
    }

    // PROCess WebSocket frames====
    if (peek_bytes < 2) {
      // Not enough data for even basic frame header
      return;
    }

    // Analyze frame header
    uint8_t first_byte = buffer[0];
    uint8_t second_byte = buffer[1];
    uint8_t opcode = first_byte & 0x0F;
    uint8_t fin = (first_byte >> 7) & 0x01;
    uint8_t masked = (second_byte >> 7) & 0x01;
    uint64_t payload_len = second_byte & 0x7F;
    int header_size = 2;

    // Extended payload length
    if (payload_len == 126)
      header_size += 2;
    else if (payload_len == 127)
      header_size += 8;

    // Masked frames require 4 additional bytes
    if (masked)
      header_size += 4;

    if (peek_bytes < header_size) {
      printf("Not enough data for full header yet (client %d)\n", client_idx);
      return; // Wait for more data
    }

    // Get actual payload length
    uint64_t actual_payload_len = payload_len;
    if (payload_len == 126) {
      // 16-bit length
      actual_payload_len = (buffer[2] << 8) | buffer[3];
    } else if (payload_len == 127) {
      // 64-bit length
      actual_payload_len = 0;
      for (int j = 2; j < 10; j++) {
        actual_payload_len = (actual_payload_len << 8) | buffer[j];
      }
    }

    uint64_t total_frame_size = header_size + actual_payload_len;

    if (peek_bytes < total_frame_size) {
      printf("Not enough data for full frame yet (client %d)\n", client_idx);
      return; // Wait for complete frame
    }

    // Now read the complete frame
    int read_bytes = recv(current_fd, buffer, total_frame_size, 0);
    if (read_bytes <= 0) {
      printf("Client %d (fd: %d) closed connection during frame read\n",
             client_idx, current_fd);
      close(current_fd);
      FD_CLR(current_fd, &(state->allset));
      state->client[client_idx] = -1;
      state->handshake_done[client_idx] = 0;
      return;
    }

    // Handle different opcodes
    switch (opcode) {
    case 0x8: // Close frame
      printf("Client %d sent Close frame\n", client_idx);
      close(current_fd);
      FD_CLR(current_fd, &(state->allset));
      state->client[client_idx] = -1;
      state->handshake_done[client_idx] = 0;
      return;

    case 0x9: // Ping frame
      printf("Client %d sent Ping frame\n", client_idx);
      // Send Pong response
      // Implementation not shown here
      break;

    case 0xA: // Pong frame
      printf("Client %d sent Pong frame\n", client_idx);
      break;

    case 0x1: // Text frame
    case 0x2: // Binary frame
      // Process the payload
      if (opcode == 0x1) { // Text frame
        cJSON *json_data = websocket_decode(buffer, read_bytes, current_fd);
        if (!json_data) {
          printf("Failed to parse JSON from client %d\n", client_idx);
          break;
        }

        const cJSON *type = cJSON_GetObjectItemCaseSensitive(json_data, "type");
        if (!type || !cJSON_IsString(type)) {
          printf("Invalid message format from client %d (missing type field)\n",
                 client_idx);
          cJSON_Delete(json_data);
          break;
        }

        printf("Received message from client %d (fd: %d), type: %s\n",
               client_idx, current_fd, type->valuestring);

        if (strcmp(type->valuestring, "join") == 0) {
          add_player_to_queue(json_data, current_fd);
        } else if (!strcmp(type->valuestring, "login") ||
                   !strcmp(type->valuestring, "signup")) {
          handle_signup_or_login(json_data, current_fd);
        } else if (!strcmp(type->valuestring, "move")) {
          handle_game_move(json_data, current_fd);
        } else if (!strcmp(type->valuestring, "reset")) {
          reset_game(json_data, current_fd);

        } else {
          printf("Unknown message type: %s\n", type->valuestring);
        }

        cJSON_Delete(json_data);
      } else {
        printf("Binary frame received from client %d - not implemented\n",
               client_idx);
      }
      break;

    default:
      printf("Unsupported opcode: %d from client %d\n", opcode, client_idx);
      break;
    }

    // Check if there might be more frames waiting
    peek_bytes = recv(current_fd, buffer, sizeof(buffer) - 1, MSG_PEEK);
    if (peek_bytes <= 0 || peek_bytes < 2) {
      continue_processing = 0; // No more complete frames available
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

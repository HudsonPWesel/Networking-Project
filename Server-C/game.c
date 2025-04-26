#include "game.h"
#include "websocket.h"
#define MAX_QUEUE 100
#define MAX_SESSIONS 10

QueuedPlayer player_queue[MAX_QUEUE];
int queue_size = 0;

GameSession sessions[MAX_SESSIONS];

void handle_game_move(cJSON *json_data, int current_fd) {
  printf("\n Handling Game Move");
  cJSON *data = cJSON_GetObjectItem(json_data, "data");
  if (!data) {
    fprintf(stderr, "Error: 'data' object missing in JSON\n");
    return;
  }

  cJSON *colItem = cJSON_GetObjectItem(data, "col");
  if (!colItem) {
    fprintf(stderr, "Error: 'col' missing in 'data' object\n");
    return;
  }

  int column = colItem->valueint;

  GameSession *game = find_session_by_fd(current_fd);
  // if (!game || !game->game_active) {
  //  create_new_game_session(4, 5);
  //  game = find_session_by_fd(current_fd);  // 🔥 reassign!
  //  if (!game) {
  //      send_error(current_fd, "No active game session.");
  //      return;
  //  }
  //}

  if (current_fd != game->current_turn_fd) {
    send_error(current_fd, "Not your turn.");
    return;
  }

  int row =
      drop_piece(game->board, column, current_fd == game->player1_fd ? 1 : 2);
  if (row == -1) {
    send_error(current_fd, "Invalid move. Column full.");
    return;
  }

  // Check for win or draw
  if (check_win(game->board, row, column)) {
    send_win_message(game, current_fd);
    game->game_active = 0;
  } else {
    switch_turn(game);
    printf("\nSending Game Update\n");
    send_game_update(game);
  }
}
void send_game_start(int fd, int player_num) {
  cJSON *msg = cJSON_CreateObject();
  cJSON_AddStringToObject(msg, "type", "start");
  cJSON_AddNumberToObject(msg, "player", player_num);
  cJSON_AddBoolToObject(msg, "yourTurn", player_num == 1);

  char *text = cJSON_PrintUnformatted(msg);
  send_websocket_message(fd, text);

  free(text);
  cJSON_Delete(msg);
}

void send_error(int fd, const char *message) {
  cJSON *response = cJSON_CreateObject();
  cJSON_AddStringToObject(response, "type", "error");
  cJSON_AddStringToObject(response, "message", message);

  char *text = cJSON_PrintUnformatted(response);
  send_websocket_message(fd, text);

  free(text);
  cJSON_Delete(response);
}
int drop_piece(int board[ROWS][COLS], int col, int player) {
  if (col < 0 || col >= COLS)
    return -1;

  for (int row = ROWS - 1; row >= 0; row--) {
    if (board[row][col] == 0) {
      board[row][col] = player;
      return row;
    }
  }
  return -1; // Col full
}

int count_in_direction(int board[ROWS][COLS], int row, int col, int drow,
                       int dcol, int player) {
  int count = 0;
  while (row >= 0 && row < ROWS && col >= 0 && col < COLS &&
         board[row][col] == player) {
    count++;
    row += drow;
    col += dcol;
  }
  return count;
}

int check_win(int board[ROWS][COLS], int row, int col) {
  int player = board[row][col];
  if (player == 0)
    return 0;

  int directions[4][2] = {
      {0, 1}, // Horizontal
      {1, 0}, // Vertical
      {1, 1}, // Diagonal \
        {1, -1}  // Diagonal /
  };

  for (int i = 0; i < 4; i++) {
    int total = count_in_direction(board, row, col, directions[i][0],
                                   directions[i][1], player) +
                count_in_direction(board, row, col, -directions[i][0],
                                   -directions[i][1], player) -
                1;

    if (total >= 4)
      return 1;
  }
  return 0;
}

void send_game_update(GameSession *game) {
  cJSON *msg = cJSON_CreateObject();
  cJSON_AddStringToObject(msg, "type", "update");

  cJSON *board = cJSON_CreateArray();
  for (int row = 0; row < ROWS; row++) {
    cJSON *row_arr = cJSON_CreateIntArray(game->board[row], COLS);
    cJSON_AddItemToArray(board, row_arr);
  }
  cJSON_AddItemToObject(msg, "board", board);

  int current_player = (game->current_turn_fd == game->player1_fd) ? 1 : 2;
  cJSON_AddNumberToObject(msg, "currentTurn", current_player);

  char *text = cJSON_PrintUnformatted(msg);
  send_websocket_message(game->player1_fd, text);
  send_websocket_message(game->player2_fd, text);
  printf("\n Game Update Sent!\n");
  // Decoded Message: {"type":"move","data":{"row":5,"col":6,"color":"rgb(237,
  // 45, 73)","player":"value1"}}

  free(text);
  cJSON_Delete(msg);
}

void send_win_message(GameSession *game, int winner_fd) {
  int winner = (winner_fd == game->player1_fd) ? 1 : 2;

  cJSON *msg = cJSON_CreateObject();
  cJSON_AddStringToObject(msg, "type", "win");
  cJSON_AddNumberToObject(msg, "winner", winner);

  char *text = cJSON_PrintUnformatted(msg);
  send_websocket_message(game->player1_fd, text);
  send_websocket_message(game->player2_fd, text);

  free(text);
  cJSON_Delete(msg);
}

GameSession *find_session_by_fd(int fd) {
  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (sessions[i].game_active &&
        (sessions[i].player1_fd == fd || sessions[i].player2_fd == fd)) {
      return &sessions[i];
    }
  }
  return NULL;
}

void switch_turn(GameSession *game) {
  if (game->current_turn_fd == game->player1_fd)
    game->current_turn_fd = game->player2_fd;
  else
    game->current_turn_fd = game->player1_fd;
}

void create_new_game_session(int fd1, int fd2) {
  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (!sessions[i].game_active) {
      sessions[i].player1_fd = fd1;
      sessions[i].player2_fd = fd2;
      sessions[i].current_turn_fd = fd2;
      memset(sessions[i].board, 0, sizeof(sessions[i].board));
      sessions[i].game_active = 1;

      send_game_start(fd1, 1);
      send_game_start(fd2, 2);

      printf("Starting game between  (fd %d) and  (fd %d)\n",
             sessions[i].player1_fd, sessions[i].player2_fd);
      return;
    }
  }
}

void send_waiting_message(int fd) {
  cJSON *msg = cJSON_CreateObject();
  cJSON_AddStringToObject(msg, "type", "waiting");
  cJSON_AddStringToObject(msg, "message",
                          "Waiting for another player to join...");

  char *text = cJSON_PrintUnformatted(msg);
  send_websocket_message(fd, text);

  free(text);
  cJSON_Delete(msg);
}

void add_player_to_queue(cJSON *json_data, int fd) {
  if (queue_size >= MAX_QUEUE) {
    printf("Matchmaking queue full.\n");
    send_error(fd, "Matchmaking queue is full. Try again later.");
    return;
  }

  // Add new player to queue
  QueuedPlayer p;
  p.fd = fd;

  cJSON *username_item = cJSON_GetObjectItem(json_data, "username");
  if (username_item && cJSON_IsString(username_item)) {
    strncpy(p.username, username_item->valuestring, sizeof(p.username) - 1);
    p.username[sizeof(p.username) - 1] = '\0'; // Safe null-terminate
  } else {
    strncpy(p.username, "anon", sizeof(p.username));
  }

  player_queue[queue_size++] = p;
  printf("Player '%s' (fd %d) added to queue. Queue size: %d\n", p.username,
         p.fd, queue_size);

  if (queue_size >= 2) {
    QueuedPlayer p1 = player_queue[0];
    QueuedPlayer p2 = player_queue[1];

    printf("Matching players: %s (fd %d) vs %s (fd %d)\n", p1.username, p1.fd,
           p2.username, p2.fd);

    create_new_game_session(p1.fd, p2.fd);

    // Shift remaining players in queue
    for (int i = 2; i < queue_size; i++) {
      player_queue[i - 2] = player_queue[i];
    }
    queue_size -= 2;
  } else {
    send_waiting_message(fd);
  }
}

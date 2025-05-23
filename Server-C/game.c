#include "game.h"
#include "database.h"
#include "websocket.h"
#include <cjson/cJSON.h>
#include <time.h>

#define MAX_QUEUE 100
#define MAX_SESSIONS 10

QueuedPlayer player_queue[MAX_QUEUE];
int queue_size = 0;

GameSession sessions[MAX_SESSIONS];

void handle_game_move(cJSON *json_data, int current_fd) {
  printf("\n Handling Game Move\n");

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

  if (!game) {
    send_error(current_fd, "Session not found.");
    return;
  }

  printf("\nNTH TURN %d\n", game->nth_turn);
  // printf("IS GAME ACTIVE %d", game->current_turn_fd);

  printf("fd(%d) | Current Player fd (%d)\n", current_fd,
         game->current_turn_fd);

  if (current_fd != game->current_turn_fd) {
    send_error(current_fd, "Not your turn.");
    printf("ERROR DETECTED");
    return;
  }

  printf("\nDROPPING PIECE==\n");
  int row =
      drop_piece(game->board, column, current_fd == game->player1_fd ? 1 : 2);
  if (row == -1) {
    send_error(current_fd, "Invalid move. Column full.");
    return;
  }

  game->nth_turn++;

  switch_turn(game);
  printf("\nSending Game Update\n");
  send_game_update(game);

  int win = 0;
  if (game->nth_turn >= 6) {
    win = check_win(game->board, row, column);
    printf("\nChecked for Win: %d\n", win);
  }

  if (win) {
    printf("\nPLAYER WON\n");
    send_win_message(game, current_fd);
    if (cJSON_GetObjectItem(data, "player")) {

      printf("Username %s", cJSON_GetObjectItem(data, "player")->valuestring);
      increase_player_score(cJSON_GetObjectItem(data, "player")->valuestring);
      game->game_active = 0;
      printf("RESETTING IS_ACTIVE = 0 %d", game->game_active);
    }
  } else if (check_tie(game->board)) {
    printf("\nTIE GAME, TIE & RESET");
    send_tie_message(game);
    reset_game(data, current_fd);
  }
}
int increase_player_score(const char *username) {
  MYSQL *conn = init_db_conn();

  if (conn == NULL) {
    fprintf(stderr, "Database connection failed.\n");
    return 1;
  }

  char query[512];
  snprintf(
      query, sizeof(query),
      "UPDATE users SET total_score = total_score + 10 WHERE username = '%s'",
      username);

  if (mysql_query(conn, query)) {
    fprintf(stderr, "Failed to update player score: %s\n", mysql_error(conn));
    mysql_close(conn);
    return 1;
  }

  if (mysql_affected_rows(conn) == 0) {
    fprintf(stderr, "No user found with username '%s'.\n", username);
    mysql_close(conn);
    return 1;
  }

  mysql_close(conn);
  return 0;
}

int check_tie(int board[ROWS][COLS]) {
  for (int r = 0; r < ROWS; r++) {
    for (int c = 0; c < COLS; c++) {
      if (board[r][c] == 0) {
        return 0; // EMPTY CELL
      }
    }
  }
  return 1;
}

void send_tie_message(GameSession *game) {

  cJSON *msg = cJSON_CreateObject();
  cJSON_AddStringToObject(msg, "type", "tie");
  char *text = cJSON_PrintUnformatted(msg);

  send_websocket_message(game->player1_fd, text);
  send_websocket_message(game->player2_fd, text);

  free(text);
  cJSON_Delete(msg);
}

void send_game_start(int fd, int player_num, int current_player_num) {
  cJSON *msg = cJSON_CreateObject();
  cJSON_AddStringToObject(msg, "type", "start");
  cJSON_AddNumberToObject(msg, "player", player_num);

  cJSON_AddBoolToObject(msg, "yourTurn", player_num == current_player_num);

  cJSON_AddNumberToObject(msg, "currentTurn", current_player_num);

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
  printf("\n Dropped Piece into col%d", col);
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
void reset_game(cJSON *json_data, int fd) {
  GameSession *game = find_session_by_fd(fd);
  printf("IS ACTIVE GAME %d", game->game_active);
  if (game->game_active)
    return;

  printf("Current FD %d", game->current_turn_fd);
  for (int row = 0; row < ROWS; row++) {
    for (int col = 0; col < COLS; col++) {
      printf("\nROW : %d | COL : %d\n", row, col);
      game->board[row][col] = 0;
    }
  }

  // send reset message to both FDs
  cJSON *response = cJSON_CreateObject();
  cJSON_AddStringToObject(response, "type", "reset");
  char *text = cJSON_PrintUnformatted(response);

  send_websocket_message(game->player1_fd, text);
  send_websocket_message(game->player2_fd, text);

  free(text);
  cJSON_Delete(response);

  srand(time(NULL));
  int first_player = (rand() % 2) + 1;

  game->current_turn_fd =
      (first_player == 1) ? game->player1_fd : game->player2_fd;
  game->game_active = 1;
  game->nth_turn = 0;

  send_game_start(game->player1_fd, 1, first_player);
  send_game_start(game->player2_fd, 2, first_player);

  printf("\nNew Game Started Between fd(%d) and fd(%d)\n", game->player1_fd,
         game->player2_fd);

  // TODO: Increment in DB
};

int check_horizontal(int board[ROWS][COLS], int row, int player) {
  int count = 0;
  for (int c = 0; c < COLS; c++) {
    if (board[row][c] == player) {
      count++;
      if (count >= 4)
        return 1;
    } else {
      count = 0;
    }
  }
  return 0;
}

int check_vertical(int board[ROWS][COLS], int col, int player) {
  int count = 0;
  for (int r = 0; r < ROWS; r++) {
    if (board[r][col] == player) {
      count++;
      if (count >= 4)
        return 1;
    } else {
      count = 0;
    }
  }
  return 0;
}

int check_diagonal_down(int board[ROWS][COLS], int row, int col, int player) {
  int start_row = row, start_col = col;
  while (start_row > 0 && start_col > 0) {
    start_row--;
    start_col--;
  }

  int count = 0;
  while (start_row < ROWS && start_col < COLS) {
    if (board[start_row][start_col] == player) {
      count++;
      if (count >= 4)
        return 1;
    } else {
      count = 0;
    }
    start_row++;
    start_col++;
  }
  return 0;
}

int check_diagonal_up(int board[ROWS][COLS], int row, int col, int player) {
  int start_row = row, start_col = col;
  while (start_row < ROWS - 1 && start_col > 0) {
    start_row++;
    start_col--;
  }

  int count = 0;
  while (start_row >= 0 && start_col < COLS) {
    if (board[start_row][start_col] == player) {
      count++;
      if (count >= 4)
        return 1;
    } else {
      count = 0;
    }
    start_row--;
    start_col++;
  }
  return 0;
}

int check_win(int board[ROWS][COLS], int row, int col) {
  int player = board[row][col];
  if (player == 0)
    return 0;

  if (check_horizontal(board, row, player))
    return 1;
  if (check_vertical(board, col, player))
    return 1;
  if (check_diagonal_down(board, row, col, player))
    return 1;
  if (check_diagonal_up(board, row, col, player))
    return 1;

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
  printf("Sending JSON: %s\n", text);
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
    if (sessions[i].player1_fd == fd || sessions[i].player2_fd == fd) {
      // sessions[i].game_active = 1;
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

void create_new_game_session(QueuedPlayer *p1, QueuedPlayer *p2) {
  for (int i = 0; i < MAX_SESSIONS; i++) {
    if (!sessions[i].game_active) {
      sessions[i].player1_fd = p1->fd;
      sessions[i].player2_fd = p2->fd;
      sessions[i].current_turn_fd = p1->fd;

      strncpy(sessions[i].player1_username, p1->username,
              sizeof(sessions[i].player1_username) - 1);
      sessions[i].player1_username[sizeof(sessions[i].player1_username) - 1] =
          '\0';

      strncpy(sessions[i].player2_username, p2->username,
              sizeof(sessions[i].player2_username) - 1);
      sessions[i].player2_username[sizeof(sessions[i].player2_username) - 1] =
          '\0';

      memset(sessions[i].board, 0, sizeof(sessions[i].board));
      sessions[i].game_active = 1;

      send_game_start(p1->fd, 1, 1);
      send_game_start(p2->fd, 2, 1);

      printf("Starting game between '%s' (fd %d) and '%s' (fd %d)\n",
             p1->username, p1->fd, p2->username, p2->fd);

      return;
    }
  }
}

void handle_disconnect(int disconnected_fd) {
  GameSession *game = find_session_by_fd(disconnected_fd);

  if (game && game->game_active) {
    printf("Player (fd: %d) disconnected. Declaring opponent as winner.\n",
           disconnected_fd);

    int winner_fd;
    const char *winner_username = NULL;

    if (disconnected_fd == game->player1_fd) {
      winner_fd = game->player2_fd;
      winner_username = game->player2_username;
    } else {
      winner_fd = game->player1_fd;
      winner_username = game->player1_username;
    }

    if (winner_fd != -1 && winner_username) {
      send_win_message(game, winner_fd);

      printf("\nPLAYER WON (username: %s)\n", winner_username);

      increase_player_score(winner_username); // <-- Update their score
    }
    send_redirect_message(winner_fd);
    game->game_active = 0; // End the game
    // TODO: REDIRECT TO WAITING ROOM ON WIN
  }
}
void send_redirect_message(int fd) {
  cJSON *msg = cJSON_CreateObject();
  cJSON_AddStringToObject(msg, "type", "redirect");
  cJSON_AddStringToObject(msg, "redirect", "waiting-room.html");

  char *text = cJSON_PrintUnformatted(msg);
  send_websocket_message(fd, text);

  free(text);
  cJSON_Delete(msg);
}
void send_leaderboard_message(cJSON *json_data, int current_fd) {
  cJSON *msg = cJSON_CreateObject();
  cJSON_AddStringToObject(msg, "type", "leaderboard");

  int scores[MAX_LEADERBOARD_ENTRIES];
  char usernames[MAX_LEADERBOARD_ENTRIES][64];
  get_leaderboard(scores, usernames);

  cJSON *leaderboard = cJSON_CreateArray();
  cJSON *users = cJSON_CreateArray();

  for (int i = 0; i < MAX_LEADERBOARD_ENTRIES; i++) {
    cJSON_AddItemToArray(leaderboard, cJSON_CreateNumber(scores[i]));
    cJSON_AddItemToArray(users, cJSON_CreateString(usernames[i]));
  }

  cJSON_AddItemToObject(msg, "scores", leaderboard);
  cJSON_AddItemToObject(msg, "users", users);

  char *text = cJSON_PrintUnformatted(msg);
  send_websocket_message(current_fd, text);

  free(text);
  cJSON_Delete(msg); // Clean up memory
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

  QueuedPlayer p;
  p.fd = fd;

  cJSON *username_item = cJSON_GetObjectItem(json_data, "username");

  if (username_item && cJSON_IsString(username_item)) {
    strncpy(p.username, username_item->valuestring, sizeof(p.username) - 1);
    p.username[sizeof(p.username) - 1] = '\0'; // Safe null-terminate
  } else {
    strncpy(p.username, "anon", sizeof(p.username));
    p.username[sizeof(p.username) - 1] = '\0'; // Also null-terminate here
  }

  player_queue[queue_size++] = p;
  printf("Player '%s' (fd %d) added to queue. Queue size: %d\n", p.username,
         p.fd, queue_size);

  if (queue_size >= 2) {
    QueuedPlayer p1 = player_queue[0];
    QueuedPlayer p2 = player_queue[1];

    printf("Matching players: %s (fd %d) vs %s (fd %d)\n", p1.username, p1.fd,
           p2.username, p2.fd);

    create_new_game_session(&p1, &p2);

    for (int i = 2; i < queue_size; i++) {
      player_queue[i - 2] = player_queue[i];
    }
    queue_size -= 2;
  } else {
    send_waiting_message(fd);
  }
}

#ifndef GAME_H
#define GAME_H
#include "websocket.h"
#include <cjson/cJSON.h>
#include <stdio.h>
#include <string.h>
#define MAX_QUEUE 100
#define MAX_SESSIONS 10
#define ROWS 6
#define COLS 7
#define USERNAME_MAX_LEN 64

typedef struct {
  int fd;
  char username[64];
} QueuedPlayer;

typedef struct {
  int board[ROWS][COLS];
  int player1_fd;
  int player2_fd;
  char player1_username[USERNAME_MAX_LEN];
  char player2_username[USERNAME_MAX_LEN];
  int current_turn_fd;
  int game_active;
  int nth_turn;
} GameSession;

extern QueuedPlayer player_queue[MAX_QUEUE];
extern int queue_size;
extern GameSession sessions[MAX_SESSIONS];

void reset_game(cJSON *json_data, int fd);
void add_player_to_queue(cJSON *json_data, int fd);
void create_new_game_session(QueuedPlayer *p1, QueuedPlayer *p2);
GameSession *find_session_by_fd(int fd);
void handle_game_move(cJSON *json_data, int fd);
void send_game_update(GameSession *game);
void send_error(int fd, const char *message);
int drop_piece(int board[ROWS][COLS], int col, int player);
void switch_turn(GameSession *game);
void send_win_message(GameSession *game, int winner_fd);
int check_win(int board[ROWS][COLS], int row, int col);
void send_game_start(int fd, int player_num, int current_player_num);
void send_tie_message(GameSession *game);
void send_leaderboard_message(cJSON *json_data, int current_fd);
int increase_player_score(const char *username);
void handle_disconnect(int disconnected_fd);

int check_horizontal(int board[ROWS][COLS], int row, int player);
int check_vertical(int board[ROWS][COLS], int col, int player);
int check_diagonal_down(int board[ROWS][COLS], int row, int col, int player);
int check_diagonal_up(int board[ROWS][COLS], int row, int col, int player);
int check_tie(int board[ROWS][COLS]);

#endif // !

#include "database.h"
#include <stdio.h>
#include <string.h>

// INITIATE SQL CONN
MYSQL *init_db_conn() {
  MYSQL *conn = mysql_init(NULL);

  if (conn == NULL) {
    fprintf(stderr, "mysql_init() failed\n");
    return NULL;
  }

  char server[16] = "localhost";
  char db_username[16] = "root";
  char db_password[16] = "P@ssw0rd";
  char database[16] = "app";

  // INIT CONN
  if (mysql_real_connect(conn, server, db_username, db_password, database, 0,
                         NULL, 0) == NULL) {
    fprintf(stderr, "mysql_real_connect() failed\n");
    mysql_close(conn);
    return NULL;
  }
  return conn;
}

int insert_user(char *username, char *hashed_password, char *token_sql) {
  MYSQL *conn = init_db_conn();
  char query[512];

  if (!token_sql) {
    printf("Insert User %s Failed!\n", username);
    return -1;
  }

  snprintf(query, sizeof(query),
           "insert into users (username, password_hash) values('%s', '%s')",
           username, hashed_password);

  if (mysql_query(conn, query)) {
    fprintf(stderr, "Insert failed: %s\n", mysql_error(conn));
    mysql_close(conn);
    return -1;
  }
  printf("User %s: was created ", username);
  return 0;
}

int insert_session_token(char *username, char *session_token) {
  MYSQL *conn = init_db_conn();

  char token_insert_query[512];

  snprintf(token_insert_query, sizeof(token_insert_query),
           "INSERT INTO sessions (username, token) VALUES ('%s', '%s')",
           username, session_token);

  if (mysql_query(conn, token_insert_query)) {
    fprintf(stderr, "Authentication query failed: %s\n", mysql_error(conn));
    mysql_close(conn);
    return 1;
  }
  return 0;
}

int get_user(char *username, char *hashed_password, MYSQL *conn) {
  char query[512];
  snprintf(query, sizeof(query),
           "SELECT * FROM users WHERE username = '%s' AND password_hash = '%s'",
           username, hashed_password);

  if (mysql_query(conn, query)) {
    fprintf(stderr, "Authentication query failed: %s\n", mysql_error(conn));
    mysql_close(conn);
    return 1;
  }

  MYSQL_RES *res = mysql_store_result(conn);
  if (res == NULL) {
    fprintf(stderr, "Failed to store result set: %s\n", mysql_error(conn));
    mysql_close(conn);
    return 1;
  }

  int num_rows = mysql_num_rows(res);
  if (num_rows == 0) {
    fprintf(stderr, "Invalid username/password\n");
    mysql_free_result(res);
    mysql_close(conn);
    return 1;
  }

  mysql_free_result(res);
  return 0;
}
int get_leaderboard(int scores[MAX_LEADERBOARD_ENTRIES],
                    char usernames[MAX_LEADERBOARD_ENTRIES][64]) {
  MYSQL *conn = init_db_conn();

  char query[512];
  snprintf(query, sizeof(query),
           "SELECT username, total_score FROM users ORDER BY total_score DESC "
           "LIMIT 10");

  if (mysql_query(conn, query)) {
    fprintf(stderr, "Leaderboard query failed: %s\n", mysql_error(conn));
    mysql_close(conn);
    return 1;
  }

  MYSQL_RES *res = mysql_store_result(conn);
  if (res == NULL) {
    fprintf(stderr, "Failed to store result set: %s\n", mysql_error(conn));
    mysql_close(conn);
    return 1;
  }

  MYSQL_ROW row;
  int index = 0;

  while ((row = mysql_fetch_row(res)) && index < MAX_LEADERBOARD_ENTRIES) {
    if (row[0] && row[1]) {
      strncpy(usernames[index], row[0], 63);
      usernames[index][63] = '\0';
      scores[index] = atoi(row[1]);
      index++;
    }
  }

  mysql_free_result(res);
  mysql_close(conn);
  return 0;
}

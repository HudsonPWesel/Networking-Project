
#ifndef DATABASE_H
#define DATABASE_H
#include <cjson/cJSON.h>
#include <mysql/mysql.h>

MYSQL *init_db_conn();
int insert_user(char *username, char *hashed_password, char *token_sql);
int insert_session_token(char *username, char *session_token);
int get_user(char *username, char *hashed_password, MYSQL *conn);

#endif

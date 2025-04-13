#include "auth.h"

cJSON *parse_json(char *decoded_message) {
  cJSON *json = cJSON_Parse(decoded_message);
  if (json == NULL) {
    printf("Error parsing JSON\n");
    return NULL;
  }
  return json;
}

void send_session_token(int client_fd, const char *session_token) {
  cJSON *response = cJSON_CreateObject();
  cJSON_AddStringToObject(response, "type", "session_token");
  cJSON_AddStringToObject(response, "session_token", session_token);
  cJSON_AddStringToObject(response, "redirect", "select.html");

  char *response_str = cJSON_Print(response);
  send_websocket_message(client_fd, response_str);

  cJSON_Delete(response);
  free(response_str);
}

void handle_signup_or_login(cJSON *json, int client_fd) {

  const cJSON *username = cJSON_GetObjectItemCaseSensitive(json, "username");
  const cJSON *password = cJSON_GetObjectItemCaseSensitive(json, "password");

  printf(" Username : %s \n  Password : %s \n", username->valuestring,
         password->valuestring);

  unsigned char hash[32];
  char hashed_password[65];

  SHA256((const unsigned char *)password->valuestring,
         strlen(password->valuestring), (unsigned char *)hash);
  sha256_to_hex((unsigned char *)hash, hashed_password);

  char session_token[32];

  srand((unsigned int)time(NULL));
  generate_random_token(session_token, TOKEN_LENGTH);

  printf("Generated Random Token : %s", session_token);

  char response[512];
  snprintf(
      response, sizeof(response),
      "Signup successful\nSet-Cookie: session_token=%s; Path=/; HttpOnly\n",
      session_token);

  send_session_token(client_fd, session_token);
}

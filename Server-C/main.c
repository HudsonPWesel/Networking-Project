#include "server.h"
#include "websocket.h"
#include <stdint.h>

#define LISTEN_BACKLOG 50

int main(int argc, char const *argv[]) {
  // MAIN.c
  int server_fd = Socket();
  ServerState state = init_ServerState(server_fd);

  for (;;) {
    state.rset = state.allset;
    int nready = select(state.maxfd + 1, &(state.rset), NULL, NULL, NULL);

    if (nready < 0) {
      // perror("None Ready");
      continue;
    }
    process_new_connection(&state);
    // process_client_data(&state);
  }

  return 0;
}

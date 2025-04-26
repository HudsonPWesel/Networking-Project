#include "server.h"
#include "websocket.h"
#include <stdint.h>

#define LISTEN_BACKLOG 50

int main(int argc, char const *argv[]) {
  // MAIN.c
  int server_fd = Socket();
  ServerState state = init_ServerState(server_fd);

  for (;;) {
    fd_set ready_set = state.rset; // Make a copy of the rset
    int nready = select(state.maxfd + 1, &ready_set, NULL, NULL, NULL);

    if (nready < 0) {
      continue;
    }

    // Handle new connections first
    if (FD_ISSET(state.listenfd, &ready_set)) {
      process_new_connection(&state);
      if (--nready <= 0)
        continue; // No need to process further if no other fds are ready
    }

    // Process client data if any client is ready
    process_client_data(&state, &ready_set);
  }

  return 0;
}

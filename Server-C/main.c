#include "server.h"
#include "websocket.h"
#include <stdint.h>

#define LISTEN_BACKLOG 50

int main(int argc, char const *argv[]) {
  int server_fd = Socket();
  ServerState state;
  init_serverstate(&state, server_fd);

  printf("WebSocket server started on port %d\n", PORT);

  for (;;) {
    state.rset = state.allset; // Reset the ready set

    printf("Waiting for activity (maxfd = %d)...\n", state.maxfd);

    int nready = select(state.maxfd + 1, &state.rset, NULL, NULL, NULL);

    if (nready < 0) {
      perror("select() error");
      continue;
    }

    printf("Select returned: %d ready descriptors\n", nready);

    // Check for conn
    if (FD_ISSET(state.listenfd, &state.rset)) {
      printf("New connection activity detected\n");
      process_new_connection(&state, &state.rset);
      nready--;
    }

    if (nready > 0) {
      printf("Processing client data (%d descriptors ready)\n", nready);
      process_client_data(&state, &state.rset);
    }
  }

  return 0;
}

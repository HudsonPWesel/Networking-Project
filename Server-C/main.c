#include "server.h"
#include "websocket.h"
#include <stdint.h>

#define LISTEN_BACKLOG 50

int main(int argc, char const *argv[]) {
  int server_fd = Socket();
  ServerState state = init_ServerState(server_fd);

  for (;;) {
    state.rset = state.allset;     // ← FIX: refresh rset from allset each time
    fd_set ready_set = state.rset; // ← copy into ready_set to work with

    int nready = select(state.maxfd + 1, &ready_set, NULL, NULL, NULL);

    if (nready < 0) {
      continue;
    }

    if (FD_ISSET(state.listenfd, &ready_set)) {
      process_new_connection(&state, &ready_set);
      if (--nready <= 0)
        continue;
    }

    process_client_data(&state, &ready_set);
  }
}

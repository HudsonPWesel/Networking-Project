const sockets = {};

export async function createSocket(username) {
  if (sockets[username] && sockets[username].readyState === WebSocket.OPEN) {
    console.warn(`Socket for ${username} already open`);
    return sockets[username];
  }
  return new Promise((resolve, reject) => {
    console.log(sockets);

    let socket = new WebSocket(`ws://10.18.102.38:9999/ws`);

    socket.onopen = () => {
      console.log("WebSocket connected as", username);
      sockets[username] = socket;
      resolve(socket);
    };

    //socket.onerror = (error) => {
    //  console.error("WebSocket error:", error);
    //  console.warn(sockets);
    //  reject(error);
    //};

    //socket.onclose = (event) => {
    //  console.log("WebSocket closed:", event);
    //  socket = null;
    //};
  });
}

export function getSocket(username) {
  return sockets[username];
}

export function closeSocket(username) {
  if (sockets[username]) {
    sockets[username].close();
    delete sockets[username];
    console.log("SOCKETS ", sockets);
  }
}


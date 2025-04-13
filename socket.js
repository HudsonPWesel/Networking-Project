let socket = null;

export function createSocket(username) {
  socket = new WebSocket('ws://10.18.102.38:9999/ws');

  socket.onopen = () => {
    console.log("WebSocket connected as", username);
  };

  socket.onerror = (error) => {
    console.error("WebSocket error:", error);
  };

  socket.onclose = (event) => {
    console.log("WebSocket closed:", event);
    socket = null;
  };

  return socket;
}

export function getSocket() {
  return socket;
}


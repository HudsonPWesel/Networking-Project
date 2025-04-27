const sockets = {};

export function createSocket(username) {
  if (sockets[username] && sockets[username].readyState === WebSocket.OPEN) {
    console.warn(`Socket for ${username} already open`);
    return sockets[username];
  }
  let socket2 = new WebSocket(`ws://192.168.20.10:9999/ws`);

  socket2.onopen = () => {
    console.log('Second socket OPEN:',);

    const joinMsg = JSON.stringify({
      type: 'join',
      username: username
    });
    console.log("Join message to send:", joinMsg);
    socket2.send(joinMsg);
    console.log("Join message sent successfully");
    //socket2.send("Hell from socket2");
  };

  socket2.onerror = (err) => {
    console.error('Second socket error:', err);
  };

  socket2.onclose = (evt) => {
    console.warn('Second socket closed:', evt);
  };

  return socket2;
  new Promise((resolve, reject) => {
    console.log("Current sockets:", sockets);
    let socket = new WebSocket(`ws://10.18.102.38:9999/ws`);
    console.log("Attempting to connect to WebSocket server...");

    // Store socket in sockets object immediately
    sockets[username] = socket;

    socket.onopen = () => {
      console.log("WebSocket connected as", username);
      console.log("Sending join message...");

      try {
        const joinMsg = JSON.stringify({
          type: 'join',
          username: username
        });
        console.log("Join message to send:", joinMsg);
        socket.send(joinMsg);
        console.log("Join message sent successfully");

        // Only resolve after successful join message sent
        resolve(socket);
      } catch (error) {
        console.error("Error sending join message:", error);
        reject(error);
      }
    };

    socket.onerror = (error) => {
      console.error("WebSocket error:", error);
      delete sockets[username]; // Clean up on error
      reject(error);
    };

    socket.onclose = (event) => {
      console.log("WebSocket closed:", event);
      delete sockets[username]; // Clean up on close
    };
  });
} export function getSocket(username) {
  return sockets[username];
}

export function closeSocket(username) {
  if (sockets[username]) {
    sockets[username].close();
    delete sockets[username];
    console.log("SOCKETS ", sockets);
  }
}


const sockets = {};

export function createSocket(username) {
  if (sockets[username] && sockets[username].readyState === WebSocket.OPEN) {
    console.warn(`Socket for ${username} already open`);
    return sockets[username];
  }
  let socket2 = new WebSocket(`ws://10.18.102.38:4242/ws`);

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


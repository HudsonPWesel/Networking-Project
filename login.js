import { createSocket } from './socket.js';

document.getElementById("loginForm").addEventListener("submit", function(event) {
  event.preventDefault();
  console.log("LOGGING IN");

  const username = document.getElementById("username").value;
  const password = document.getElementById("password").value;

  const socket = createSocket(username);

  socket.onopen = () => {
    console.log("WebSocket is open, sending login...");
    socket.send(JSON.stringify({ type: "login", username, password }));
  };

  socket.onmessage = (e) => {
    console.log("Received after login:", e.data);
  };
});

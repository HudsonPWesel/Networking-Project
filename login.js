import { createSocket, closeSocket } from './socket.js';

document.getElementById("loginForm").addEventListener("submit", async function(event) {
  event.preventDefault();

  const username = document.getElementById("username").value;
  const password = document.getElementById("password").value;

  try {
    const socket = await createSocket(username);
    console.log("WebSocket is open, sending login...");

    socket.send(JSON.stringify({ type: "login", username, password }));

    socket.onmessage = (e) => {
      console.log("Received after login:", e.data);
      const data = JSON.parse(e.data);
      localStorage.setItem("username", username);

      if (data.redirect) {
        window.location.href = data.redirect;
      }
    };

    socket.onclose(() => { closeSocket(username); })

  } catch (err) {
    console.error("Failed to connect socket:", err);
  }
});


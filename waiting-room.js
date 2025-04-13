import { createSocket, getSocket } from './socket.js';

const playerCountEl = document.getElementById("playerCount");
const statusEl = document.getElementById("status");


window.addEventListener("load", async () => {
  const username = localStorage.getItem("username");
  if (!username) {
    window.location.href = "/login.html";
    return;
  }

  try {
    console.log("Trying to create socetk")
    await createSocket(username);
    let socket = getSocket(username);

    socket.onopen = () => {
      console.log("Connected to WebSocket as", username);
      socket.send(JSON.stringify({ type: "join-waiting-room", username }));
    };

    socket.onmessage = (event) => {
      const message = JSON.parse(event.data);
      console.log("Received:", message);

      if (message.type === "waiting-room-update") {
        playerCountEl.textContent = message.playerCount;
      } else if (message.type === "game-start") {
        statusEl.textContent = "Match found! Starting game...";
        window.location.href = "/game.html";
      }
    };

    socket.onerror = (err) => {
      console.error("WebSocket error", err);
      statusEl.textContent = "Connection error!";
    };

    socket.onclose = () => {
      statusEl.textContent = "Disconnected from server.";
    };
  } catch (err) {
    console.error("Socket conn failed:", err);
  }
});


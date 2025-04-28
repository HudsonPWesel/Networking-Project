import { createSocket } from './socket.js';

const playerCountEl = document.getElementById("playerCount");
const statusEl = document.getElementById("status");
let socket;

window.addEventListener("load", async () => {
  const username = localStorage.getItem("username");
  if (!username) {
    window.location.href = "/login.html";
    return;
  }

  try {
    console.log("Trying to create socetk")
    socket = createSocket(username);

    socket.onopen = () => {
      console.log("Connected to WebSocket as", username);
      socket.send(JSON.stringify({ type: "join", username }));
    };

    socket.onmessage = (event) => {
      const message = JSON.parse(event.data);
      console.log("Received:", message);

      if (message.type === "waiting-room-update") {
        playerCountEl.textContent = message.playerCount;
      } else if (message.type === "start") {
        statusEl.textContent = "Match found! Starting game...";
        window.location.href = "/Networking-Project/game.html";
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


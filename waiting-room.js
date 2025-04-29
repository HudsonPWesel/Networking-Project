import { createSocket } from './socket.js';

const playerCountEl = document.getElementById("playerCount");
const statusEl = document.getElementById("status");
const leaderboardListEl = document.getElementById("leaderboardList");
let socket;

window.addEventListener("load", async () => {
  const username = localStorage.getItem("username");
  if (!username) {
    window.location.href = "/login.html";
    return;
  }

  try {
    console.log("Trying to create socket");
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
      } else if (message.type === "leaderboard") {
        updateLeaderboard(message.users, message.scores);
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
    console.error("Socket connection failed:", err);
  }
});

function updateLeaderboard(users, scores) {
  leaderboardListEl.innerHTML = '';

  for (let i = 0; i < users.length; i++) {
    const li = document.createElement("li");
    li.textContent = `${users[i]}: ${scores[i]} pts`;
    if (users[i] === localStorage.getItem('username'))
      li.style.fontWeight = 'bold';
    leaderboardListEl.appendChild(li);
  }
}


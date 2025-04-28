import { createSocket } from './socket.js'

let socket;

function getSocketStatus() {
  setInterval(() => {
    if (socket) {
      console.log('Current WebSocket State:', {
        readyState: socket.readyState,
        readyStateDescription: ['Connecting', 'Open', 'Closing', 'Closed'][socket.readyState]
      });
    }
  }, 5000);
}

function main() {
  const username = document.getElementById("username").value;
  const password = document.getElementById("password").value;
  socket = createSocket(username);

  socket.onopen = () => {
    console.log("WebSocket connection established");


    console.log("Sending login credentials:", { username, password });
    localStorage.setItem("username", username);
    socket.send(JSON.stringify({ type: "signup", username, password }));

    // Get the session_id from cookies and send for authentication
    //const session_id = document.cookie.split("=")[1];
    //if (session_id) {
    //  socket.send(JSON.stringify({ type: "auth", session_id }));
    //} else {
    //  console.error("Session ID is missing.");
    //}
  };

  socket.onmessage = function(event) {
    const data = JSON.parse(event.data);
    const { type, redirect_url } = data;
    console.log(`Message Received: ${data}`);

    if (type === "session_token") {
      localStorage.setItem("session_token", data.session_token);

      if (redirect_url)
        window.location.href = redirect_url;
    }
  };

  socket.onerror = (error) => {
    console.error("WebSocket Error:", error);
  };

  socket.onclose = (event) => {
    console.log("WebSocket closed:", event.code, event.reason);
  };
}

document.getElementById("loginForm").addEventListener("submit", function(e) {
  e.preventDefault();
  main();
});


var socket = null;

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

function initWebSocket() {
  socket = new WebSocket("ws://10.18.102.38:9999");

  socket.onopen = () => {
    console.log("WebSocket connection established");

    // Get the session_id from cookies and send for authentication
    const session_id = document.cookie.split("=")[1];
    if (session_id) {
      socket.send(JSON.stringify({ type: "auth", session_id }));
    } else {
      console.error("Session ID is missing.");
    }
  };

  socket.onmessage = function(event) {
    const data = JSON.parse(event.data);

    if (data.type === "session_token") {
      // Store token
      localStorage.setItem("session_token", data.session_token);

      // Redirect
      if (data.redirect) {
        window.location.href = data.redirect;
      }
    }

  };


  socket.onerror = (error) => {
    console.error("WebSocket Error:", error);
  };

  socket.onclose = (event) => {
    console.log("WebSocket closed:", event.code, event.reason);
  };
}

document.getElementById("loginForm").addEventListener("submit", function(event) {
  event.preventDefault();

  const username = document.getElementById("username").value;
  const password = document.getElementById("password").value;

  initWebSocket();

  socket.onopen = () => {
    console.log("Sending login credentials:", { username, password });
    socket.send(JSON.stringify({ type: "signup", username, password }));
  };
});


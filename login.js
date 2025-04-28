import { createSocket } from './socket.js';

document.getElementById("loginForm").addEventListener("submit", async function(event) {
  event.preventDefault();

  const username = document.getElementById("username").value;
  const password = document.getElementById("password").value;

  try {
    const socket = createSocket(username);

    socket.onopen = () => {
      console.log("WebSocket is open, sending login...");

      socket.send(JSON.stringify({ type: "login", username, password }));

      socket.onmessage = (e) => {
        console.log("Received after login:", e.data);
        const data = JSON.parse(e.data);


        if (data.type === "session_token") {

          localStorage.setItem("session_token", data.session_token);
          localStorage.setItem("username", data.username);

          if (data.redirect)
            window.location.href = data.redirect;

        }


      };
    };

    socket.onerror = (err) => {
      console.error("WebSocket error:", err);
    };

    // socket.onclose(() => { closeSocket(username); })

  } catch (err) {
    console.error("Failed to connect socket:", err);
  }
});


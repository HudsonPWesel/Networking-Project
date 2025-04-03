const socket = new WebSocket('ws://10.18.102.38:9999/ws');

var form = document.getElementById("form");
var input = document.getElementById("input");

function handleForm(event) { event.preventDefault(); console.log("Sent Data: ", input.value); socket.send(input.value); }
form.addEventListener('submit', handleForm);

// Add comprehensive logging for each WebSocket event
socket.onopen = () => {
  console.log('WebSocket connection established');
  console.log('ReadyState:', socket.readyState);
  socket.send("Initial connection message");
};

socket.onmessage = (event) => {
  console.log("Received message from server:", event.data);
};

socket.onerror = (error) => {
  console.error("WebSocket Error Details:", {
    error: error,
    readyState: socket.readyState
  });
};

socket.onclose = (event) => {
  console.log("WebSocket closed with details:", {
    code: event.code,
    reason: event.reason,
    wasClean: event.wasClean
  });
};

// Optional: Check connection status periodically
setInterval(() => {
  console.log('Current WebSocket State:', {
    readyState: socket.readyState,
    // 0: CONNECTING, 1: OPEN, 2: CLOSING, 3: CLOSED
    readyStateDescription: ['Connecting', 'Open', 'Closing', 'Closed'][socket.readyState]
  });
}, 5000);

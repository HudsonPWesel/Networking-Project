const socket = new WebSocket('ws://localhost:4242/ws');

// Add comprehensive logging for each WebSocket event
socket.onopen = (event) => {
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

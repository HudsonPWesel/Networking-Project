let socket = new WebSocket('ws://127.0.0.1:4444');

if (socket){
  //console.log(socket);
  console.log('websocket created!');
}

socket.onopen = (event) => {
  console.log('connection established and socket is open');
  socket.send("here's some text that the server is urgently awaiting!");
};

socket.onmessage = (event) => { 
  console.log("message from server ", event.data); 
};

const form = document.getElementById("form");

form.addEventListener("submit", event => {
  event.preventDefault();
  const payload = document.querySelector("input[name=message]").value;
  console.log(`Message : ${payload}`);
  socket.send(payload);
});

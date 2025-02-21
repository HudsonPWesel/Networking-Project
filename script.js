let socket = new WebSocket('ws://10.240.233.102:4444');

if (socket){
  //console.log(socket);
  console.log('websocket created!');
}

socket.onopen = (event) => {
  console.log('connection established');
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

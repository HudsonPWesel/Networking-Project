let socket = new WebSocket('ws://10.240.233.102:1234');

if (socket){
  console.log(socket);
  console.log('WebSocket Created!');
}

socket.addEventListener("open", (event) => {
  socket.send("Hello Server!");
});

socket.addEventListener("message", (event) => {
  console.log("Message from server ", event.data);
});



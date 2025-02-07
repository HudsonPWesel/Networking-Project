let socket = new WebSocket('ws://10.240.233.102:4444');

if (socket){
  console.log(socket);
  console.log('WebSocket Created!');
}

// onopen
socket.onopen = () => {console.log('Connection Established')};

socket.send('Hello!');

socket.addEventListener("message", (event) => {
  console.log("Message from server ", event.data);
});



let socket = new WebSocket('ws://10.240.233.102:4444');

if (socket){
  console.log(socket);
  console.log('WebSocket Created!');
}

socket.onopen = (event) => {
  console.log('Connection Established');
  socket.send("Here's some text that the server is urgently awaiting!");
};

socket.onmessage = (event) => { 
  console.log("Message from server ", event.data); 
};

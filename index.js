import { createSocket, getSocket } from './socket.js'

await createSocket();
let socket = getSocket(sessionStorage.getItem("username"));
console.log(socket)

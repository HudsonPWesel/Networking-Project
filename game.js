import { createSocket, getSocket } from './socket.js';

const playerName = sessionStorage.getItem("username");
if (!playerName) {
  alert("No username found. Please login first.");
  window.location.href = "/login.html";
}

// Assign or retrieve a unique color for the player
let playerColor = sessionStorage.getItem("playerColor");
const colors = [
  'rgb(237, 45, 73)', 'rgb(106, 168, 79)', 'rgb(255, 126, 185)', 'rgb(225, 245, 245)',
  'rgb(76, 141, 255)', 'rgb(255, 187, 82)', 'rgb(155, 86, 255)', 'rgb(60, 60, 60)'
];

if (!playerColor) {
  playerColor = colors[Math.floor(Math.random() * colors.length)];
  sessionStorage.setItem("playerColor", playerColor);
}

let isMyTurn = playerName === "value1";
let table = $('table tr');


async function setup() {
  console.log(playerName);
  await createSocket(playerName);
  const socket = getSocket(playerName);

  socket.onmessage = (e) => {
    console.log("Recieved Message");

    const msg = JSON.parse(e.data);
    console.log(msg);

    if (msg.type === "start") {
      isMyTurn = msg.yourTurn === playerName;
      $('h3').text(`${msg.turn}: your turn`);
    }

    if (msg.type === "move") {
      const { row, col, color, player } = msg.data;
      changeColor(row, col, color);

      // Check for win
      if (horizontalWinCheck() || verticalWinCheck() || diagonalWinCheck()) {
        gameEnd(player);
      } else if (tieCheck()) {
        gameEnd(1);
      }

      isMyTurn = msg.nextTurn === playerName;
      $('h3').text(`${msg.nextTurn}: your turn`);
    }
  };
};


$('.board button').on('click', function() {
  if (!isMyTurn) {
    alert("Wait for your turn!");
    return;
  }

  const col = $(this).closest("td").index();
  const row = checkBottom(col);

  if (row === undefined) {
    alert("Column full!");
    return;
  }

  changeColor(row, col, playerColor);
  const socket = getSocket(playerName);
  socket.send(JSON.stringify({
    type: "move",
    data: { row, col, color: playerColor, player: playerName }
  }));

  isMyTurn = false;
});

// --- Helper functions ---
function changeColor(rowIndex, colIndex, color) {
  return table.eq(rowIndex).find('td').eq(colIndex).find('button').css('background-color', color);
}

function returnColor(rowIndex, colIndex) {
  return table.eq(rowIndex).find('td').eq(colIndex).find('button').css('background-color');
}

function winHighlight(rowIndex, colIndex) {
  return table.eq(rowIndex).find('td').eq(colIndex).find('button').css({ 'border-width': '9px' });
}

function checkBottom(colIndex) {
  for (let row = 5; row >= 0; row--) {
    if (returnColor(row, colIndex) === rgb(138, 138, 138)) {
      return row;
    }
  }
}

function tieCheck() {
  let defaultCount = 0;
  for (let col = 0; col <= 6; col++) {
    for (let row = 0; row <= 7; row++) {
      if (returnColor(row, col) === rgb(138, 138, 138)) {
        defaultCount++;
      }
    }
  }
  if (defaultCount == 0) {
    return true;
  }
  return false;
}

function colorMatchCheck(one, two, three, four) {
  return one === two && one === three && one === four && one !== 'rgb(138, 138, 138)' && one !== undefined;
}

function horizontalWinCheck() {
  for (let row = 0; row < 6; row++) {
    for (let col = 0; col < 4; col++) {
      if (colorMatchCheck(returnColor(row, col), returnColor(row, col + 1), returnColor(row, col + 2), returnColor(row, col + 3))) {
        [0, 1, 2, 3].forEach(i => winHighlight(row, col + i));
        return true;
      }
    }
  }
  return false;
}

function verticalWinCheck() {
  for (let col = 0; col < 7; col++) {
    for (let row = 0; row < 3; row++) {
      if (colorMatchCheck(returnColor(row, col), returnColor(row + 1, col), returnColor(row + 2, col), returnColor(row + 3, col))) {
        [0, 1, 2, 3].forEach(i => winHighlight(row + i, col));
        return true;
      }
    }
  }
  return false;
}

function diagonalWinCheck() {
  for (let col = 0; col < 5; col++) {
    for (let row = 0; row < 6; row++) {
      if (colorMatchCheck(returnColor(row, col), returnColor(row + 1, col + 1), returnColor(row + 2, col + 2), returnColor(row + 3, col + 3))) {
        [0, 1, 2, 3].forEach(i => winHighlight(row + i, col + i));
        return true;
      }
      if (row >= 3 && colorMatchCheck(returnColor(row, col), returnColor(row - 1, col + 1), returnColor(row - 2, col + 2), returnColor(row - 3, col + 3))) {
        [0, 1, 2, 3].forEach(i => winHighlight(row - i, col + i));
        return true;
      }
    }
  }
  return false;
}

function gameEnd(winner) {
  if (winner == 1) {
    $('h1').text("It's a tie!").css("fontSize", "50px");
  } else {
    $('h1').text(`${winner} wins!`).css("fontSize", "50px");
  }
  $('.board button').prop('disabled', true);
  $('.resetButton').prop('disabled', true);
  $('h3').fadeOut();
}

setup();

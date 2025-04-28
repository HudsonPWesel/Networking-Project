import { createSocket } from './socket.js';

const playerName = localStorage.getItem("username");

if (!playerName) {
  alert("No username found. Please login first.");
  window.location.href = "/login.html";
}

// --- Setup player color ---
let playerColor = localStorage.getItem("playerColor");
const colors = [
  'rgb(237, 45, 73)', 'rgb(106, 168, 79)', 'rgb(255, 126, 185)', 'rgb(225, 245, 245)',
  'rgb(76, 141, 255)', 'rgb(255, 187, 82)', 'rgb(155, 86, 255)', 'rgb(60, 60, 60)'
];

if (!playerColor) {
  playerColor = colors[Math.floor(Math.random() * colors.length)];
  localStorage.setItem("playerColor", playerColor);
}

// --- Game variables ---
const DEFAULT_COLOR = "rgb(138, 138, 138)";
let table = $('table tr');
let isMyTurn = false;
let socket;
let playerNumber;


function reset_handler() {
  $('h1').text(`Welcome to Connect Four!`);
  //e.preventDefault();
  socket.send(JSON.stringify({ 'type': 'reset', 'playerNumber': playerNumber }));
  console.log("Resetting Board");
  document.querySelectorAll('#board_btn').forEach(boardButton => { console.log(boardButton); boardButton.style.backgroundColor = DEFAULT_COLOR; boardButton.style.borderWidth = '2px'; });
}

async function setup() {
  console.log(playerName);
  try {
    socket = createSocket(playerName);

    console.log("SOCKET IN GAMEJS", socket);
    socket.onmessage = (e) => {
      const msg = JSON.parse(e.data);
      console.log(e.data);

      if (msg.type === "start") {
        isMyTurn = msg.yourTurn;
        playerNumber = msg.player;
        $('h3').text(`Player ${msg.currentTurn}: your turn`);
        document.querySelectorAll('#board_btn').forEach(boardButton => {
          boardButton.disabled = false;
        });
      }

      else if (msg.type === "update") {
        const { board } = msg;
        for (let row = 0; row < board.length; row++) {
          for (let col = 0; col < board[row].length; col++) {
            const slot = board[row][col];
            if (slot == playerNumber) {
              changeColor(row, col, playerColor);
            } else if (slot != 0) {
              changeColor(row, col, 'rgb(255, 187, 82)');
            }
          }
        }
        isMyTurn = msg.currentTurn === playerNumber;
        let turnColor = isMyTurn ? playerColor : 'black';
        $('h3').text(`Player ${msg.currentTurn}: your turn`).css('color', turnColor);
        console.log(`Is My Turn : ${isMyTurn}`);
      }

      else if (msg.type === "reset") {
        console.log("RESET MESSAGE RECEIVED");

        document.querySelectorAll('#board_btn').forEach(boardButton => {
          boardButton.style.backgroundColor = DEFAULT_COLOR;
          boardButton.disabled = true;
          boardButton.style.borderWidth = '2px';
          $('h1').text(`Welcome to Connect Four!`);
          console.log('H3 TEXT');
          $('h3').text(`Player ${msg.currentTurn}: your turn`);
          $('h3').text(`Player ${msg.currentTurn}: your turn`).css('display', 'block');

        });

        $('h3').text(`New Game Starting...`).css('color', 'black');

        isMyTurn = false;
        playerNumber = null; // Reset player number

      }

      else if (msg.type === "win") {
        console.log("WINNER FOUND");
        endGame(msg.winner);
      }
      else if (msg.type === "tie") {
        //tieCheck()
        console.log("Game Tied");
        reset_handler()
      }
    };

  } catch (error) {
    console.error("Failed to connect:", error);

  }
}

document.getElementById("reset_btn").addEventListener("click", reset_handler);

$('.board button').on('click', function() {
  console.log(isMyTurn);
  if (!isMyTurn) {
    alert("Wait for your turn!");
    return;
  }

  const col = $(this).closest("td").index();
  const row = checkBottom(col);

  if (!checkTie()) {
    if (row === -1) {
      alert("Column full!");
      return;
    }

    changeColor(row, col, playerColor);
    socket.send(JSON.stringify({ type: "move", data: { row, col, color: playerColor, player: playerName } }));
    isMyTurn = false;

  } else {
    socket.send(JSON.stringify({ type: "tie", data: { color: playerColor, player: playerName } }));
  }
});

function changeColor(rowIndex, colIndex, color) {
  return table.eq(rowIndex).find('td').eq(colIndex).find('button').css('background-color', color);
}

function returnColor(rowIndex, colIndex) {
  if (rowIndex >= 0) {
    return table.eq(rowIndex).find('td').eq(colIndex).find('button').css('background-color');
  }
}

function winHighlight(rowIndex, colIndex) {
  return table.eq(rowIndex).find('td').eq(colIndex).find('button').css({ 'border-width': '9px' });
}

function checkBottom(colIndex) {
  for (let row = 5; row >= 0; row--) {
    if (returnColor(row, colIndex) === DEFAULT_COLOR) {
      return row;
    }
  }
  return -1;
}

function checkTie() {
  for (let col = 0; col < 7; col++) {
    for (let row = 0; row < 6; row++) { // 6 rows, not 7 dumbass
      if (returnColor(row, col) === DEFAULT_COLOR) {
        return false;
      }
    }
  }
  return true;
}

function colorMatchCheck(one, two, three, four) {
  return one === two && one === three && one === four && one !== DEFAULT_COLOR && one !== undefined;
}

function horizontalWinCheck() {
  for (let row = 0; row < 6; row++) {
    for (let col = 0; col < 4; col++) {
      if (colorMatchCheck(
        returnColor(row, col),
        returnColor(row, col + 1),
        returnColor(row, col + 2),
        returnColor(row, col + 3)
      )) {
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
      if (colorMatchCheck(
        returnColor(row, col),
        returnColor(row + 1, col),
        returnColor(row + 2, col),
        returnColor(row + 3, col)
      )) {
        [0, 1, 2, 3].forEach(i => winHighlight(row + i, col));
        return true;
      }
    }
  }
  return false;
}

function diagonalWinCheck() {
  for (let row = 0; row < 6; row++) {
    for (let col = 0; col < 7; col++) {
      // Diagonal right-down
      if (row + 3 < 6 && col + 3 < 7 &&
        colorMatchCheck(
          returnColor(row, col),
          returnColor(row + 1, col + 1),
          returnColor(row + 2, col + 2),
          returnColor(row + 3, col + 3)
        )) {
        [0, 1, 2, 3].forEach(i => winHighlight(row + i, col + i));
        return true;
      }
      // Diagonal left-down
      if (row + 3 < 6 && col - 3 >= 0 &&
        colorMatchCheck(
          returnColor(row, col),
          returnColor(row + 1, col - 1),
          returnColor(row + 2, col - 2),
          returnColor(row + 3, col - 3)
        )) {
        [0, 1, 2, 3].forEach(i => winHighlight(row + i, col - i));
        return true;
      }
    }
  }
  return false;
}

function endGame(winner) {
  //if (winner == 1) {
  //  $('h1').text("It's a tie!").css("fontSize", "50px");
  //} 
  diagonalWinCheck();
  verticalWinCheck();
  horizontalWinCheck();

  $('h1').text(`${winner} wins!`).css("fontSize", "50px");

  $('.board button').prop('disabled', true);
  //$('.resetButton').prop('disabled', true);
  $('h3').fadeOut();
  //TODO : RESET EVERYTHIGN ON CLIKC OF RESTE BUTTON

}

setup();


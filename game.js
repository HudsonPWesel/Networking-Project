import socket from './socket.js'

document.getElementById("board_btn").addEventListener("click", () => { sendMove('34') });

function sendMove(move) {
  const socket = getSocket();
  if (socket && socket.readyState === WebSocket.OPEN) {
    socket.send(JSON.stringify({ type: "move", data: move }));
  } else {
    console.warn("WebSocket not ready");
  }
}

var colors = ['rgb(237, 45, 73)',
  'rgb(106, 168, 79)',
  'rgb(255, 126, 185)',
  'rgb(225, 245, 245)',

  'rgb(76, 141, 255)',
  'rgb(255, 187, 82)',
  'rgb(155, 86, 255)',
  'rgb(60, 60, 60)'];

var player1 = "Player 1";
var player1ColorVal = 'rgb(237, 45, 73)';
var player1ColorName = "red";

var player2 = "Player 2";
var player2ColorVal = '#40E0D0';
var player2ColorName = "teal";

var game_on = true;
var table = $('table tr');

const params = new URLSearchParams(window.location.search);

// Extract specific parameters
player1 = params.get('username');
player1ColorVal = params.get('colorValue');
player1ColorName = params.get('colorName').toLowerCase();
console.log(player1ColorVal);

//Scenario where player doesn't choose a color
if (player1ColorVal == '') {
  player1ColorVal = '#964B00';
  player1ColorName = 'brown';
}

//Reports to console the first winning tile (from left-right; up-down)
function reportWin(rowNum, colNum) {
  console.log("You won starting at this row,col");
  console.log(rowNum);
  console.log(colNum);
}

// Change the color of a button
function changeColor(rowIndex, colIndex, color) {
  return table.eq(rowIndex).find('td').eq(colIndex).find('button').css('background-color', color);
}

// Report Back to current color of a button
function returnColor(rowIndex, colIndex) {
  return table.eq(rowIndex).find('td').eq(colIndex).find('button').css('background-color');
}

//Highlights the winning connect-4
function winHighlight(rowIndex, colIndex) {
  return table.eq(rowIndex).find('td').eq(colIndex).find('button').css({ 'border-width': '9px' });
}

// Take in column index, returns the bottom row that is still gray
function checkBottom(colIndex) {
  var colorReport = returnColor(5, colIndex);
  for (var row = 5; row > -1; row--) {
    colorReport = returnColor(row, colIndex);
    if (colorReport === 'rgb(138, 138, 138)') {
      return row;
    }
  }
}

// Check to see if 4 inputs are the same color
function colorMatchCheck(one, two, three, four) {
  return (one === two && one === three && one === four && one !== 'rgb(138, 138, 138)' && one !== undefined);
}

// Check for Horizontal Wins
function horizontalWinCheck() {
  for (var row = 0; row < 6; row++) {
    for (var col = 0; col < 4; col++) {
      if (colorMatchCheck(returnColor(row, col), returnColor(row, col + 1), returnColor(row, col + 2), returnColor(row, col + 3))) {
        console.log('horiz');
        winHighlight(row, col), winHighlight(row, col + 1), winHighlight(row, col + 2), winHighlight(row, col + 3);
        reportWin(row, col);
        return true;
      } else {
        continue;
      }
    }
  }
}

// Check for Vertical Wins
function verticalWinCheck() {
  for (var col = 0; col < 7; col++) {
    for (var row = 0; row < 3; row++) {
      if (colorMatchCheck(returnColor(row, col), returnColor(row + 1, col), returnColor(row + 2, col), returnColor(row + 3, col))) {
        console.log('vertical');
        winHighlight(row, col), winHighlight(row + 1, col), winHighlight(row + 2, col), winHighlight(row + 3, col);
        reportWin(row, col);
        return true;
      } else {
        continue;
      }
    }
  }
}

// Check for Diagonal Wins
function diagonalWinCheck() {
  for (var col = 0; col < 5; col++) {
    for (var row = 0; row < 7; row++) {
      if (colorMatchCheck(returnColor(row, col), returnColor(row + 1, col + 1), returnColor(row + 2, col + 2), returnColor(row + 3, col + 3))) {
        console.log('diag');
        winHighlight(row, col), winHighlight(row + 1, col + 1), winHighlight(row + 2, col + 2), winHighlight(row + 3, col + 3);
        reportWin(row, col);
        return true;
      } else if (colorMatchCheck(returnColor(row, col), returnColor(row - 1, col + 1), returnColor(row - 2, col + 2), returnColor(row - 3, col + 3))) {
        console.log('diag');
        winHighlight(row, col), winHighlight(row - 1, col + 1), winHighlight(row - 2, col + 2), winHighlight(row - 3, col + 3);
        reportWin(row, col);
        return true;
      } else {
        continue;
      }
    }
  }
}

// Game End
function gameEnd(winningPlayer) {
  $('h3').fadeOut('slow');
  $('h2').fadeOut('slow');
  $('h1').text(winningPlayer + " has won!").css("fontSize", "50px");
  $('.board button').prop('disabled', true);
  $('.resetButton').prop('disabled', true);
}

// Start with Player One
var currentPlayer = 1;
var currentName = player1;
var currentColor = player1ColorVal;

// Start with Player One
// TODO: Update player name to be their login
$('h3').text(player1 + ": it is your turn, please pick a column to drop your " + player1ColorName + " chip.");

$('.board button').on('click', function() {

  // Recognize what column was chosen
  var col = $(this).closest("td").index();

  // Get back bottom available row to change
  var bottomAvail = checkBottom(col);

  // Drop the chip in that column at the bottomAvail Row
  changeColor(bottomAvail, col, currentColor);

  // Check for a win or a tie.
  if (horizontalWinCheck() || verticalWinCheck() || diagonalWinCheck()) {
    gameEnd(currentName);
  }

  // If no win or tie, continue to next player
  currentPlayer = currentPlayer * -1;

  // Re-Check who the current Player is.
  if (currentPlayer === 1) {
    currentName = player1;
    $('h3').text(currentName + ": it is your turn, please pick a column to drop your " + player1ColorName + " chip.");
    currentColor = player1ColorVal;
  } else {
    currentName = player2;
    $('h3').text(currentName + ": it is your turn, please pick a column to drop your " + player2ColorName + " chip.");
    currentColor = player2ColorVal;
  }

})

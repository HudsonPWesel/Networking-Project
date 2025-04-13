const allColors = document.getElementsByClassName("colorButton");
const colorValue = document.getElementById('colorValue');
const colorName = document.getElementById('colorName');
var btn = document.getElementById("submitButton");
console.log(btn);
var colors = ["Red", "Green", "Pink", "White", "Blue", "Yellow", "Purple", "Black"];
import { socket, username } from "./login.js"

//Determines name + rgb value of player's chip color
$('.colorButton').on('click', function() {
  const buttonsArray = Array.from(allColors);
  const index = buttonsArray.indexOf(this);

  const rgbValue = this.style.backgroundColor;
  const chosenColor = colors[index];

  colorValue.value = rgbValue;
  colorName.value = chosenColor;
  $('h2').text("Chosen color:  " + chosenColor);
})



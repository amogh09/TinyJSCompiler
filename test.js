var swap = function(firstNumber, secondNumber){
  var temporaryVariable = firstNumber;
  firstNumber = secondNumber;
  secondNumber = temporaryVariable;
  return firstNumber
}

var a = 30;
var b = 20;
swap(a, b);

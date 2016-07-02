const fs = require('fs');
const esprima = require('esprima');

//Tokens
const TOKENS = [
  "ArrayExpression", "ArrayPattern", "ArrowFunctionExpression",
  "AssignmentExpression", "AssignmentPattern", "BinaryExpression",
  "BlockStatement", "BreakStatement", "CallExpression",
  "CatchClause", "ClassBody", "ClassDeclaration",
  "ClassExpression", "ComputedMemberExpression", "ConditionalExpression",
  "ContinueStatement", "DebuggerStatement", "Directive",
  "DoWhileStatement", "EmptyStatement", "ExportAllDeclaration",
  "ExportDefaultDeclaration", "ExportNamedDeclaration", "ExportSpecifier",
  "ExpressionStatement", "ForInStatement", "ForOfStatement",
  "ForStatement", "FunctionDeclaration", "FunctionExpression",
  "Identifier", "IfStatement", "ImportDeclaration",
  "ImportDefaultSpecifier", "ImportNamespaceSpecifier", "ImportSpecifier",
  "LabeledStatement", "Literal", "MetaProperty",
  "MethodDefinition", "NewExpression", "ObjectExpression",
  "ObjectPattern", "Program", "Property",
  "RegexLiteral", "RestElement", "ReturnStatement",
  "SequenceExpression", "SpreadElement", "StaticMemberExpression",
  "Super", "SwitchCase", "SwitchStatement",
  "TaggedTemplateExpression", "TemplateElement", "TemplateLiteral",
  "ThisExpression", "ThrowStatement", "TryStatement",
  "UnaryExpression", "UpdateExpression", "VariableDeclaration",
  "VariableDeclarator", "WhileStatement", "WithStatement",
  "YieldExpression"
];

//define constants
Const = {
  TRUE : "true",
  FALSE : "false",
  UNDEFINED : "undefined",
  NULL : "null"
}

//Nemonics
Nemonic = {
  NUMBER : "number",
  FIXNUM : "fixnum",
  STRING : "string",
  GETGLOBALOBJ : "getglobalobj",
  SETFL : "setfl",
  SETGLOBAL : "setglobal",
  GETGLOBAL : "getglobal",
  SETA : "seta",
  GETA : "geta",
  RET : "ret",
  ADD : "add",
  SUB : "sub",
  MUL : "mul",
  DIV : "div",
  MOD : "mod",
  BITAND : "bitand",
  BITOR : "bitor",
  LEFTSHIFT : "leftshift",
  RIGHTSHIFT : "rightshift",
  UNSIGNEDRIGHTSHIFT : "unsignedrightshift",
  SPECCONST : "specconst",
  CONST : "const",
  MOVE  : "move",
  JUMP  : "jump",
  JUMPTRUE : "jumptrue",
  JUMPFALSE : "jumpfalse",
  LESSTHAN : "lessthan",
  LESSTHANEQUAL : "lessthanequal"
}

//Location
Location = {
  LOC_ARGUMENTS : "LOC_ARGUMENTS",
  LOC_REGISTER : "LOC_REGISTER",
  LOC_CATCH : "LOC_CATCH",
  LOC_LOCAL : "LOC_LOCAL",
  LOC_ARG : "LOC_ARG",
  LOC_GLOBAL : "LOC_GLOBAL"
}

//Environment
function Environment(name, location, level, offset, index, next){
  this.name = name;
  this.location = location;
  this.level = level;
  this.offset = offset;
  this.index = index;
  this.next = next;
}

//function table
function FunctionTable(node, rho, existClosure, level, callEntry, sendEntry){
  this.node = node;
  this.rho = rho;
  this.existClosure = existClosure;
  this.level = level;
  this.callEntry = callEntry;
  this.sendEntry = sendEntry;
}

//Byte code
function Bytecode(nemonic, label, flag, callEntry, sendEntry, bcType){
  this.nemonic = nemonic;
  this.label = label;
  this.flag = flag;
  this.callEntry = callEntry;
  this.sendEntry = sendEntry;
  this.bcType = bcType;
}

//Byte code types follow
function dval(dst, val){
  this.dst = dst;
  this.val = val;
}
function ival(dst, val){
  this.dst = dst;
  this.val = val;
}
function str(dst, theString){
  this.dst = dst;
  this.theString = theString;
}
function regnum(r1, n1){
  this.r1 = r1;
  this.n1 = n1;
}
function num(n1){
  this.n1 = n1;
}
function unireg(r1){
  this.r1 = r1;
}
function bireg(r1, r2){
  this.r1 = r1;
  this.r2 = r2;
}
function trireg(r1, r2, r3){
  this.r1 = r1;
  this.r2 = r2;
  this.r3 = r3;
}
function num(n){
  this.n1 = n;
}
function constType(dst, cons){
  this.dst = dst;
  this.cons = cons;
}

var arithNemonic = function(operator){
  switch (operator) {
    case "+":
      return Nemonic.ADD;
    case "-":
      return Nemonic.SUB;
    case "*":
      return Nemonic.MUL;
    case "/":
      return Nemonic.DIV;
    case "%":
      return Nemonic.MOD;
    case "&":
      return Nemonic.BITAND;
    case "|":
      return Nemonic.BITOR;
    case "<<":
      return Nemonic.LEFTSHIFT;
    case ">>":
      return Nemonic.RIGHTSHIFT;
    case ">>>":
      return Nemonic.UNSIGNEDRIGHTSHIFT;
    case "<":
      return Nemonic.LESSTHAN;
    case "<=":
      return Nemonic.LESSTHANEQUAL;
    default:
      return "UNKNOWN";
  }
}

const FL_MAX = 1500;
const MAX_BYTECODE = 10000;
const FL_TABLE_MAX = 201;
var frameLinkTable = new Array(FL_TABLE_MAX);
var maxFuncFl = 0;
var currentLabel = 1;
var currentTable = 0;
var variableNum = new Array(FL_TABLE_MAX);
var codeNum = new Array(FL_TABLE_MAX);
var currentFunction = 1;
var usedRegTable = new Array(FL_MAX);
var touchRegTable = new Array(FL_MAX);
var currentCodeNum = 0;
var currentCode = 0;
var bytecode = new Array(FL_TABLE_MAX);

//main code begins
var inputFileName;
var outputFileName;
var sourceCode;
var writeStream;

var initBytecode = function(){
  for(var i=0; i<FL_TABLE_MAX; i++){
    bytecode[i] = new Array(MAX_BYTECODE)
    for(var j=0; j<MAX_BYTECODE; j++){
      bytecode[i][j] = null;
    }
  }
}

var initVariableNumCodeNum = function(){
  for(var i=0; i<variableNum.length; i++){
    variableNum[i] = 0;
    codeNum[i] = 0;
  }
}

var initRegTbl = function(){
  for(var i=0; i<FL_MAX; i++){
    // releaseReg(currentTable, i);
    usedRegTable[i] = 0;
    touchRegTable[i] = 0;
  }

  usedRegTable[0] = 1;
  touchRegTable[0] = 1;
  usedRegTable[1] = 1;
  touchRegTable[1] = 1;
}

var searchUnusedReg = function(){
  var i = 0;
  for(i=0; usedRegTable[i] != 0 && i < FL_MAX; i++);
  usedRegTable[i] = 1;
  touchRegTable[i] = 1;
  return i;
}

var setBytecodeUniReg = function(nemonic, flag, r1){
  var u = new unireg(r1);
  bytecode[currentCode][currentCodeNum++] = new Bytecode(nemonic, null, flag, null, null, u);
}

var setBytecodeBiReg = function(nemonic, flag, r1, r2){
  var bi = new bireg(r1, r2);
  bytecode[currentCode][currentCodeNum++] = new Bytecode(nemonic, null, flag, null, null, bi);
}

var setBytecodeTriReg = function(nemonic, flag, r1, r2, r3){
  var tri = new trireg(r1, r2, r3);
  bytecode[currentCode][currentCodeNum++] = new Bytecode(nemonic, null, flag, null, null, tri);
}

var setBytecodeNum = function(nemonic, flag, n1){
  var n = new num(n1);
  bytecode[currentCode][currentCodeNum++] = new Bytecode(nemonic, null, flag, null, null, n);
}

var setBytecodeString = function(nemonic, flag, dst, theString){
  var s = new str(dst, theString);
  bytecode[currentCode][currentCodeNum++] = new Bytecode(nemonic, null, flag, null, null, s);
}

var setBytecodeDVal = function(nemonic, flag, dst, doubleVal){
  var d = new dval(dst, doubleVal);
  bytecode[currentCode][currentCodeNum++] = new Bytecode(nemonic, null, flag, null, null, d);
}

var setBytecodeIVal = function(nemonic, flag, dst, intVal){
  var i = new ival(dst, intVal);
  bytecode[currentCode][currentCodeNum++] = new Bytecode(nemonic, null, flag, null, null, i);
}

var setBytecodeCons = function(nemonic, flag, dst, consVal){
  var c = new constType(dst, consVal);
  bytecode[currentCode][currentCodeNum++] = new Bytecode(nemonic, null, flag, null, null, c);
}

var setBytecodeRegnum = function(nemonic, flag, r1, n1){
  var rn = new regnum(r1, n1);
  bytecode[currentCode][currentCodeNum++] = new Bytecode(nemonic, null, flag, null, null, rn);
}

var setBytecodeNum = function(nemonic, flag, n1){
  var n = new num(n1);
  bytecode[currentCode][currentCodeNum++] = new Bytecode(nemonic, null, flag, null, null, n);
}

var setBytecodeFl = function(){
  for(var i = 0; i < currentCodeNum; i++){
    if(bytecode[currentCode][i].flag === 2){
      switch (bytecode[currentCode][i].nemonic) {
        //TODO: Case 'MOVE'
        case Nemonic.SETFL:
          bytecode[currentCode][i].flag = 0;
          bytecode[currentCode][i].bcType.n1 += frameLinkTable[currentCode];
          break;
        default:
          throw new Error("setBytecodeFl Unexpected expression: '" + bytecode[i][j].nemonic + "'\n");
      }
    }
  }
}

var highestRegTouched = function(){
  for(var i = 0; touchRegTable[i] != 0; i++);
  return i-1;
}

var dispatchLabel = function(jumpLine, labelLine){
  switch(bytecode[currentCode][jumpLine].nemonic){
    case Nemonic.JUMPTRUE:
    case Nemonic.JUMPFALSE:
      bytecode[currentCode][jumpLine].flag = 0;
      bytecode[currentCode][jumpLine].bcType.n1 = labelLine - jumpLine;
      break;
    //TODO: Add 'try'
    case Nemonic.JUMP:
      bytecode[currentCode][jumpLine].flag = 0;
      bytecode[currentCode][jumpLine].bcType.n1 = labelLine - jumpLine;
      break;
    default:
      throw new Error("Nemonic: '" + bytecode[currentCode][jumpLine].nemonic + "' NOT IMPLEMENTED YET." + "\n");
  }
}

var calculateFrameLink = function(){
  return highestRegTouched() + maxFuncFl + 4;
}

var environmentLookup = function(rho, currentLevel, name){
  while(rho != null){
    if(name === rho.name){
      return {
        level : currentLevel - rho.level,
        offset : rho.offset,
        index : rho.index,
        location : rho.location
      }
      rho = rho.next;
    }

    return {
      level : null,
      offset : null,
      index : null,
      location : Location.LOC_GLOBAL
    };
  }
}

var printBytecode = function(bytecode, num, writeStream){
  writeStream.write("funcLength " + num + "\n");

  for(var i = 0; i < num; i++){
    writeStream.write("callentry 0\n");
    writeStream.write("sendentry " + (i==0?0:1) + "\n");
    writeStream.write("numberOfLocals " + variableNum[i] + "\n");
    writeStream.write("numberOfInstruction " + codeNum[i] + "\n");

    for(var j = 0; j < codeNum[i]; j++){
      //console.log("j = " + j + ", bytecode[i][j]: " + bytecode[i][j].nemonic);
      if(bytecode[i][j].label != null && bytecode[i][j].label != 0){
        writeStream.write(bytecode[i][j].label + ":\n");
      }

      switch (bytecode[i][j].nemonic) {
        case Nemonic.NUMBER:
          writeStream.write(bytecode[i][j].nemonic + " " + bytecode[i][j].bcType.dst + " " + bytecode[i][j].bcType.val + "\n");
          break;
        case Nemonic.FIXNUM:
          writeStream.write(bytecode[i][j].nemonic + " " + bytecode[i][j].bcType.dst + " " + bytecode[i][j].bcType.val + "\n");
          break;
        case Nemonic.STRING:
          writeStream.write(bytecode[i][j].nemonic + " " + bytecode[i][j].bcType.dst + ' "' + bytecode[i][j].bcType.theString + '"' + "\n");
          break;

        //TODO: Add MOD, BITAND, BITOR, LEFTSHIFT, RIGHTSHIFT, UNSIGNEDRIGHTSHIFT
        case Nemonic.ADD:
        case Nemonic.SUB:
        case Nemonic.MUL:
        case Nemonic.DIV:
        case Nemonic.MOD:
        case Nemonic.BITAND:
        case Nemonic.BITOR:
        case Nemonic.LEFTSHIFT:
        case Nemonic.RIGHTSHIFT:
        case Nemonic.UNSIGNEDRIGHTSHIFT:
          writeStream.write(bytecode[i][j].nemonic + " " + bytecode[i][j].bcType.r1 + " " + bytecode[i][j].bcType.r2 + " " + bytecode[i][j].bcType.r3 + "\n");
          break;

        case Nemonic.GETGLOBAL:
        case Nemonic.SETGLOBAL:
          writeStream.write(bytecode[i][j].nemonic + " " + bytecode[i][j].bcType.r1 + " " + bytecode[i][j].bcType.r2 + "\n");
          break;

        case Nemonic.GETGLOBALOBJ:
        case Nemonic.GETA:
        case Nemonic.SETA:
          writeStream.write(bytecode[i][j].nemonic + " " + bytecode[i][j].bcType.r1 + "\n");
          break;

        case Nemonic.RET:
          writeStream.write(bytecode[i][j].nemonic + "\n");
          break;

        case Nemonic.SETFL:
          writeStream.write(bytecode[i][j].nemonic + " " + bytecode[i][j].bcType.n1 + "\n");
          break;

        case Nemonic.CONST:
        case Nemonic.SPECCONST:
          writeStream.write(bytecode[i][j].nemonic + " " + bytecode[i][j].bcType.dst + " " + bytecode[i][j].bcType.cons + "\n");
          break;

        case Nemonic.MOVE:
          if(bytecode[i][j].flag == 2){
            throw new Error("Nemonic: '" + bytecode[i][j].nemonic + "' with flag '2' NOT IMPLEMENTED YET." + "\n");
          } else {
            writeStream.write(bytecode[i][j].nemonic + " " + bytecode[i][j].bcType.r1 + " " + bytecode[i][j].bcType.r2 + "\n");
          }
          break;

        case Nemonic.JUMPTRUE:
        case Nemonic.JUMPFALSE:
          if(bytecode[i][j].flag == 1){
            writeStream.write(bytecode[i][j].nemonic + " " + bytecode[i][j].bcType.r1 + " L" + bytecode[i][j].bcType.n1 + "\n");
          } else {
            writeStream.write(bytecode[i][j].nemonic + " " + bytecode[i][j].bcType.r1 + " " + bytecode[i][j].bcType.n1 + "\n");
          }
          break;

        //TODO: case 'TRY'
        case Nemonic.JUMP:
          if(bytecode[i][j].flag == 1){
            writeStream.write(bytecode[i][j].nemonic + " L" + bytecode[i][j].bcType.n1 + "\n");
          } else {
            writeStream.write(bytecode[i][j].nemonic + " " + bytecode[i][j].bcType.n1 + "\n");
          }
          break;

        case Nemonic.LESSTHAN:
        case Nemonic.LESSTHANEQUAL:
          writeStream.write(bytecode[i][j].nemonic + " " + bytecode[i][j].bcType.r1 + " " + bytecode[i][j].bcType.r2 + " " + bytecode[i][j].bcType.r3 + "\n");
          break;

        default:
          throw new Error("Nemonic: '" + bytecode[i][j].nemonic + "' NOT IMPLEMENTED YET. i=" + "\n");
      }
    }
  }
}

var compileAssignment = function(str, rho, src, currentLevel){
  var level, offset, index, location;
  var obj = environmentLookup(rho, currentLevel, str);

  level = obj.level;
  offset = obj.offset;
  index = obj.index;
  location = obj.location;

  switch (location) {
    case Location.LOC_REGISTER:
      //console.log("Location.LOC_REGISTER not implemented yet.");
      break;
    case Location.LOC_GLOBAL:
      //tn is a register
      var tn = searchUnusedReg();
      setBytecodeString(Nemonic.STRING, 0, tn, str);
      setBytecodeBiReg(Nemonic.SETGLOBAL, 0, tn, src);
      // TODO: releaseReg(currentTable, tn);
      break;
    default:
  }
}

var compileBytecode = function(root, rho, dst, tailFlag, currentLevel){
  if(root == null){
    return;
  }

  if(TOKENS.indexOf(root.type) == -1){
    //console.log("Unexpected token " + root.type + "\n");
    return;
  }

  switch (root.type) {
    case "Program":
      for(var i = 0; i < root.body.length; i++){
        compileBytecode(root.body[i], rho, dst, tailFlag, currentLevel);
      }
      break;

    case "BinaryExpression":
      var t1 = searchUnusedReg();
      var t2 = searchUnusedReg();
      compileBytecode(root.left, rho, t1, 0, currentLevel);
      compileBytecode(root.right, rho, t2, 0, currentLevel);
      switch (root.operator) {
        case "<":
        case "<=":
          setBytecodeTriReg(root.operator=="<"?Nemonic.LESSTHAN:Nemonic.LESSTHANEQUAL,
                            0, dst, t1, t2);
          break;
        case ">":
        case ">=":
          setBytecodeTriReg(root.operator==">"?Nemonic.LESSTHAN:Nemonic.LESSTHANEQUAL,
                            0, dst, t2, t1);
          break;
        default:
          setBytecodeTriReg(arithNemonic(root.operator), 0, dst, t1, t2);
      }
      break;

    case "AssignmentExpression":
      if(root.left.type == "Identifier"){
        var name = root.left.name;
        var t1 = searchUnusedReg();
        var t2 = searchUnusedReg();
        if(root.operator == "="){
          compileBytecode(root.right, rho, dst, 0, currentLevel);
          compileAssignment(name, rho, dst, currentLevel);
        } else {
          compileBytecode(root.left, rho, t1, 0, currentLevel);
          compileBytecode(root.right, rho, t2, 0, currentLevel);
          setBytecodeTriReg(arithNemonic(root.operator), 0, dst, t1, t2);
          compileAssignment(name, rho, dst, currentLevel);
        }
      }
      break;

    case "VariableDeclaration":
      for(var i = 0; i < root.declarations.length; i++){
        compileBytecode(root.declarations[i], rho, dst, tailFlag, currentLevel);
      }
      break;

    case "VariableDeclarator":
      if(root.init != null){
        var name = root.id.name;
        compileBytecode(root.init, rho, dst, 0, currentLevel);
        compileAssignment(name, rho, dst, currentLevel);
      }
      break;

    case "Literal":
      if(root.value == null){
        setBytecodeCons(Nemonic.SPECCONST, 0, dst, Const.NULL);
        break;
      }

      switch (typeof root.value) {
        case "number":
          var val = root.value;
          if(Number.isSafeInteger(val)){
            setBytecodeIVal(Nemonic.FIXNUM, 0, dst, val);
          } else{
            setBytecodeDVal(Nemonic.NUMBER, 0, dst, val);
          }
          break;
        case "boolean":
          if(root.value){
            setBytecodeCons(Nemonic.SPECCONST, 0, dst, Const.TRUE);
          } else {
            setBytecodeCons(Nemonic.SPECCONST, 0, dst, Const.FALSE);
          }
          break;
        default:
          throw new Error(typeof root.value + ": NOT IMPLEMENTED YET.");
      }
      break;

    case "ThisExpression":
      setBytecodeBiReg(Nemonic.MOVE, 0, dst, 1);
      break;

    case "Identifier":
      var name = root.name;
      var tn = searchUnusedReg();
      var level, offset, index, location;
      var obj = environmentLookup(rho, currentLevel, str);

      level = obj.level;
      offset = obj.offset;
      index = obj.index;
      location = obj.location;

      switch (location) {
        //TODO: Add LOC_LOCAL, LOC_REGISTER, and LOC_ARG
        case Location.LOC_GLOBAL:
          setBytecodeString(Nemonic.STRING, 0, tn, name);
          setBytecodeBiReg(Nemonic.GETGLOBAL, 0, dst, tn);
          break;
        default:
          throw new Error("Unexpected case in case 'Identifier'");
      }
      break;

    case "ExpressionStatement":
      compileBytecode(root.expression, rho, dst, 0, currentLevel);
      break;
    case "SequenceExpression":
      for(var i=0; i<root.expressions.length; i++){
        compileBytecode(root.expressions[i], rho, dst, 0, currentLevel);
      }
      break;

    case "IfStatement":
      var elseExists = true;
      if(root.alternate === null)
        elseExists = false;
      var t = searchUnusedReg();
      var l1 = currentLabel++;
      var l2 = currentLabel++;
      var j1, j2;
      compileBytecode(root.test, rho, t, 0, currentLevel);

      j1 = currentCodeNum;
      setBytecodeRegnum(Nemonic.JUMPFALSE, 1, t, l1);
      compileBytecode(root.consequent, rho, dst, 0, currentLevel);
      if(elseExists){
        j2 = currentCodeNum;
        setBytecodeNum(Nemonic.JUMP, 1, l2);
      }
      l1 = currentCodeNum;
      if(elseExists){
        compileBytecode(root.alternate, rho, dst, 0, currentLevel);
        l2 = currentCodeNum;
      }
      dispatchLabel(j1, l1);
      if(elseExists){
        dispatchLabel(j2, l2);
      }
      break;

    case "BlockStatement":
      for(var i=0; i<root.body.length; i++){
        compileBytecode(root.body[i], rho, dst, tailFlag, currentLevel);
      }
      break;

    default:
      throw new Error(root.type + ": NOT IMPLEMENTED YET.");
  }
}

//compile the source code
var processSourceCode = function(){
  var rho = new Environment();
  var globalDst;
  initRegTbl();
  initBytecode();
  initVariableNumCodeNum();

  setBytecodeUniReg(Nemonic.GETGLOBALOBJ, 0, 1);
  setBytecodeNum(Nemonic.SETFL, 2, 0);
  globalDst = searchUnusedReg();

  var ast = esprima.parse(sourceCode);
  compileBytecode(ast, rho, globalDst, 0, 0);
  setBytecodeUniReg(Nemonic.SETA, 0, globalDst);
  setBytecodeUniReg(Nemonic.RET, 0, 0);
  frameLinkTable[0] = calculateFrameLink();
  setBytecodeFl();

  //TODO: Insert function compilation part

  codeNum[0] = currentCodeNum;

  printBytecode(bytecode, currentFunction, writeStream);
  writeStream.end();
}

if(process.argv.length > 2){
  //input file specified
  inputFileName = process.argv[2];
  sourceCode = fs.readFileSync(inputFileName);
  outputFileName = inputFileName.substring(0, inputFileName.lastIndexOf('.')) + ".sbc";
  writeStream = fs.createWriteStream(outputFileName);
  processSourceCode();
} else {
  //no input file specified, use stdin & stdout
  writeStream = fs.createWriteStream(outputFileName, {fd: 1});
  //called on input from stdin
  process.stdin.on('readable',function() {
    var vText = process.stdin.read();
    if (sourceCode == null && vText != null)
      sourceCode = vText;
    else if (vText != null)
      sourceCode = sourceCode + vText;
  });

  //when input from stdin finishes
  process.stdin.on('end',function() {
    processSourceCode();
  });
}

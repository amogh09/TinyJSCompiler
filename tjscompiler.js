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

const MAX_SAFE_INTEGER = 9007199254740991;
const MIN_SAFE_INTEGER = -9007199254740991;
const MAX_VALUE = new Number(1.79E+308);
const MIN_VALUE = new Number(5e-324);

//Register: stored as integers
// function Register(r){
//   const reg = r;  //register number
// }

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
  RET : "ret"
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
function unireg(r1){
  this.r1 = r1;
}
function bireg(r1, r2){
  this.r1 = r1;
  this.r2 = r2;
}
function num(n){
  this.n1 = n;
}

const FL_MAX = 1500;
const MAX_BYTECODE = 10000;
const FL_TABLE_MAX = 201;
var frameLinkTable = new Array(FL_TABLE_MAX);
var maxFuncFl = 0;
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

var setBytecodeNum = function(nemonic, flag, n1){
  var n = new num(n1);
  bytecode[currentCode][currentCodeNum++] = new Bytecode(nemonic, null, flag, null, null, n);
}

var setBytecodeString = function(nemonic, flag, dst, theString){
  var s = new str(dst, theString);
  bytecode[currentCode][currentCodeNum++] = new Bytecode(nemonic, null, flag, null, null, s);
}

var setBytecodeBiReg = function(nemonic, flag, r1, r2){
  var bi = new bireg(r1, r2);
  bytecode[currentCode][currentCodeNum++] = new Bytecode(nemonic, null, flag, null, null, bi);
}

var setBytecodeDVal = function(nemonic, flag, dst, doubleVal){
  var d = new dval(dst, doubleVal);
  bytecode[currentCode][currentCodeNum++] = new Bytecode(nemonic, null, flag, null, null, d);
}

var setBytecodeIVal = function(nemonic, flag, dst, intVal){
  var i = new ival(dst, intVal);
  bytecode[currentCode][currentCodeNum++] = new Bytecode(nemonic, null, flag, null, null, i);
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

        default:
          throw new Error("Nemonic: '" + bytecode[i][j].nemonic + "' NOT IMPLEMENTED YET." + "\n");
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

  //console.log("TOKEN TYPE: " + root.type);

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

    case "VariableDeclaration":
      for(var i = 0; i < root.declarations.length; i++){
        // //console.log(root.declarationObject.assign = require('object-assign')s[i].type);
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
      switch (typeof root.value) {
        case "number":
          var val = root.value;
          if(Number.isSafeInteger(val)){
            setBytecodeIVal(Nemonic.FIXNUM, 0, dst, val);
          } else{
            setBytecodeDVal(Nemonic.NUMBER, 0, dst, val);
          }
          break;
        default:
          throw new Error(typeof root.value + ": NOT IMPLEMENTED YET.");
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
  // //console.log(JSON.stringify(ast));
  compileBytecode(ast, rho, globalDst, 0, 0);
  setBytecodeUniReg(Nemonic.SETA, 0, globalDst);
  setBytecodeUniReg(Nemonic.RET, 0, 0);
  frameLinkTable[0] = calculateFrameLink();
  setBytecodeFl();

  //TODO: Insert function compilation part

  codeNum[0] = currentCodeNum;

  printBytecode(bytecode, currentFunction, writeStream);

  // //console.log(ast["type"]);
  // writeStream.write(JSON.stringify(ast) + "\n");
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

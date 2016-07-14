const fs = require('fs');
const esprima = require('esprima');

var TOKENLIST = [
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
  "YieldExpression", "MEMBEREXPRESSION"
];

Tokens = {
  PROGRAM : "Program",
  ARRAYEXPRESSION : "ArrayExpression",
  ARRAYPATTERN : "ArrayPattern",
  ARROWFUNCTIONEXPRESSION : "ArrowFunctionExpression",
  ASSIGNMENTEXPRESSION : "AssignmentExpression",
  ASSIGNMENTPATTERN : "AssignmentPattern",
  BINARYEXPRESSION : "BinaryExpression",
  BLOCKSTATEMENT : "BlockStatement",
  BREAKSTATEMENT : "BreakStatement",
  CALLEXPRESSION : "CallExpression",
  CATCHCLAUSE : "CatchClause",
  CLASSBODY : "ClassBody",
  CLASSDECLARATION : "ClassDeclaration",
  CLASSEXPRESSION : "ClassExpression",
  COMPUTEDMEMBEREXPRESSION : "ComputedMemberExpression",
  CONDITIONALEXPRESSION : "ConditionalExpression",
  CONTINUESTATEMENT : "ContinueStatement",
  DEBUGGERSTATEMENT : "DebuggerStatement",
  DIRECTIVE : "Directive",
  DOWHILESTATEMENT : "DoWhileStatement",
  EMPTYSTATEMENT : "EmptyStatement",
  EXPORTALLDECLARATION : "ExportAllDeclaration",
  EXPORTDEFAULTDECLARATION : "ExportDefaultDeclaration",
  EXPORTNAMEDDECLARATION : "ExportNamedDeclaration",
  EXPORTSPECIFIER : "ExportSpecifier",
  EXPRESSIONSTATEMENT : "ExpressionStatement",
  FORINSTATEMENT : "ForInStatement",
  FOROFSTATEMENT : "ForOfStatement",
  FORSTATEMENT : "ForStatement",
  FUNCTIONDECLARATION : "FunctionDeclaration",
  FUNCTIONEXPRESSION : "FunctionExpression",
  IDENTIFIER : "Identifier",
  IFSTATEMENT : "IfStatement",
  IMPORTDECLARATION : "ImportDeclaration",
  IMPORTDEFAULTSPECIFIER : "ImportDefaultSpecifier",
  IMPORTNAMESPACESPECIFIER : "ImportNamespaceSpecifier",
  IMPORTSPECIFIER : "ImportSpecifier",
  LABELEDSTATEMENT : "LabeledStatement",
  LITERAL : "Literal",
  METAPROPERTY : "MetaProperty",
  METHODDEFINITION : "MethodDefinition",
  NEWEXPRESSION : "NewExpression",
  OBJECTEXPRESSION : "ObjectExpression",
  OBJECTPATTERN : "ObjectPattern",
  PROGRAM : "Program",
  PROPERTY : "Property",
  REGEXLITERAL : "RegexLiteral",
  RESTELEMENT : "RestElement",
  RETURNSTATEMENT : "ReturnStatement",
  SEQUENCEEXPRESSION : "SequenceExpression",
  SPREADELEMENT : "SpreadElement",
  STATICMEMBEREXPRESSION : "StaticMemberExpression",
  SUPER : "Super",
  SWITCHCASE : "SwitchCase",
  SWITCHSTATEMENT : "SwitchStatement",
  TAGGEDTEMPLATEEXPRESSION : "TaggedTemplateExpression",
  TEMPLATEELEMENT : "TemplateElement",
  TEMPLATELITERAL : "TemplateLiteral",
  THISEXPRESSION : "ThisExpression",
  THROWSTATEMENT : "ThrowStatement",
  TRYSTATEMENT : "TryStatement",
  UNARYEXPRESSION : "UnaryExpression",
  UPDATEEXPRESSION : "UpdateExpression",
  VARIABLEDECLARATION : "VariableDeclaration",
  VARIABLEDECLARATOR : "VariableDeclarator",
  WHILESTATEMENT : "WhileStatement",
  WITHSTATEMENT : "WithStatement",
  YIELDEXPRESSION : "YieldExpression",
  MEMBEREXPRESSION : "MemberExpression"
}

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
  LESSTHANEQUAL : "lessthanequal",
  NEWARGS : "newargs",
  MAKECLOSURE : "makeclosure",
  GETARG : "getarg",
  GETLOCAL : "getlocal",
  CALL : "call",
  SETLOCAL : "setlocal",
  SETARG : "setarg",
  NEW : "new",
  NEWSEND : "newsend",
  SETARRAY : "setarray",
  NEW : "new",
  NEWSEND: "newsend",
  SETARRAY: "setarray",
  GETIDX: "getidx",
  GETPROP: "getprop",
  SEND : "send",
  SETPROP: "setprop",
  INSTANCEOF: "instanceof"
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
function variable(n1, n2, r1){
  this.n1 = n1;
  this.n2 = n2;
  this.r1 = r1;
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
const gStringArray = "Array"
const gStringObject = "Object"
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
var functionTable = new Array(FL_TABLE_MAX);

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

var initRegTbl = function(currentLevel){
  for(var i=0; i<FL_MAX; i++){
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

var setBytecodeVariable = function(nemonic, flag, n1, n2, r1){
  var v = new variable(n1, n2, r1);
  bytecode[currentCode][currentCodeNum++] = new Bytecode(nemonic, null, flag, null, null, v);
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
        case Nemonic.SETFL:
          bytecode[currentCode][i].flag = 0;
          bytecode[currentCode][i].bcType.n1 += frameLinkTable[currentCode];
          break;
        case Nemonic.MOVE:
          bytecode[currentCode][i].flag = 0;
          bytecode[currentCode][i].bcType.r1 += frameLinkTable[currentCode];
          break;
        default:
          throw new Error("setBytecodeFl Unexpected expression: '" + bytecode[currentCode][i].nemonic + "'\n");
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

function environmentExpand(name, location, level, offset, index, rho){
  var e = new Environment(name, location, level, offset, index, rho);
  return e;
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

function addFunctionTable(node, rho, level){
  var existClosure = level > 1;
  var f = new FunctionTable(node, rho, existClosure, level, null, null);
  functionTable[currentFunction] = f;
  // currentFunction++;
  return currentFunction++ - 1;
}

function searchFunctionDefinition(root){
  if(root == undefined || root == null){
    return false;
  }

  if(TOKENLIST.indexOf(root.type) == undefined){
    return false;
  }

  switch(root.type){
    case Tokens.BINARYEXPRESSION:
      return searchFunctionDefinition(root.left) || searchFunctionDefinition(root.right)
    case Tokens.EXPRESSIONSTATEMENT:
      return searchFunctionDefinition(root.expression);
    case Tokens.ASSIGNMENTEXPRESSION:
      return searchFunctionDefinition(root.left || root.right);
    case Tokens.WHILESTATEMENT:
    case Tokens.DOWHILESTATEMENT:
      return searchFunctionDefinition(root.test || root.body);
    case Tokens.RETURNSTATEMENT:
      return searchFunctionDefinition(root.argument);
    case Tokens.IFSTATEMENT:
      return searchFunctionDefinition(root.test || root.consequent || root.alternate);
    case Tokens.FORSTATEMENT:
      return searchFunctionDefinition(root.init || root.test || root.update || root.body);
    case Tokens.VARIABLEDECLARATION:
      var f = false;
      for(var i=0; i<root.declarations.length && !f; i++){
        f = searchFunctionDefinition(root.declarations[i]);
      }
      return f;
    case Tokens.FUNCTIONDECLARATION:
      return true;
    case Tokens.LITERAL:
      return false;
    case Tokens.VARIABLEDECLARATOR:
      return searchFunctionDefinition(root.id || root.init);
    case Tokens.MEMBEREXPRESSION:
      return searchUseArguments(root.object || root.property);
    case Tokens.IDENTIFIER:
      return false;
    case Tokens.LITERAL:
      return false;
    case Tokens.BLOCKSTATEMENT:
      var f = false;
      for(var i=0; i<root.body.length && !f; i++){
        f = searchFunctionDefinition(root.body[i]);
      }
      return f;
    default:
      throw new Error("searchFunctionDefinition(): '" + root.type + "' NOT IMPLEMENTED YET.\n");
  }
}

function searchUseArguments(root){
  if(root == undefined || root == null){
    return false;
  }

  if(TOKENLIST.indexOf(root.type) == undefined){
    return false;
  }

  switch(root.type){
    case Tokens.BINARYEXPRESSION:
      return searchUseArguments(root.left) || searchUseArguments(root.right)
    case Tokens.EXPRESSIONSTATEMENT:
      return searchUseArguments(root.expression);
    case Tokens.ASSIGNMENTEXPRESSION:
      return searchUseArguments(root.left || root.right);
    case Tokens.WHILESTATEMENT:
    case Tokens.DOWHILESTATEMENT:
      return searchUseArguments(root.test || root.body);
    case Tokens.RETURNSTATEMENT:
      return searchUseArguments(root.argument);
    case Tokens.IFSTATEMENT:
      return searchUseArguments(root.test || root.consequent || root.alternate);
    case Tokens.FORSTATEMENT:
      return searchUseArguments(root.init || root.test || root.update || root.body);
    case Tokens.VARIABLEDECLARATION:
      var f = false;
      for(var i=0; i<root.declarations.length && !f; i++){
        f = searchUseArguments(root.declarations[i]);
      }
      return f;
    case Tokens.FUNCTIONDECLARATION:
      return false;
    case Tokens.LITERAL:
      return false;
    case Tokens.VARIABLEDECLARATOR:
      return searchUseArguments(root.id || root.init);
    case Tokens.MEMBEREXPRESSION:
      return searchUseArguments(root.object || root.property);
    case Tokens.IDENTIFIER:
      return root.name === "arguments";
    case Tokens.THISEXPRESSION:
    case Tokens.LITERAL:
      return false;
    case Tokens.BLOCKSTATEMENT:
      var f = false;
      for(var i=0; i<root.body.length && !f; i++){
        f = searchUseArguments(root.body[i]);
      }
      return f;


    default:
      throw new Error("searchUseArguments(): '" + root.type + "' NOT IMPLEMENTED YET.\n");
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

        case Nemonic.NEW:
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

        case Nemonic.SETARRAY:
        case Nemonic.GETPROP:
        case Nemonic.SETPROP:
        case Nemonic.LESSTHAN:
        case Nemonic.INSTANCEOF:
        case Nemonic.LESSTHANEQUAL:
          writeStream.write(bytecode[i][j].nemonic + " " + bytecode[i][j].bcType.r1 + " " + bytecode[i][j].bcType.r2 + " " + bytecode[i][j].bcType.r3 + "\n");
          break;

        case Nemonic.MAKECLOSURE:
          writeStream.write(bytecode[i][j].nemonic + " " + bytecode[i][j].bcType.r1 + " " + bytecode[i][j].bcType.n1 + "\n");
          break;

        case Nemonic.NEWARGS:
          writeStream.write(bytecode[i][j].nemonic + "\n");
          break;

        case Nemonic.GETARG:
        case Nemonic.GETLOCAL:
          writeStream.write(bytecode[i][j].nemonic + " " + bytecode[i][j].bcType.r1 + " " + bytecode[i][j].bcType.n1 + " " + bytecode[i][j].bcType.n2 + "\n");
          break;

        case Nemonic.SEND:
        case Nemonic.NEWSEND:
        case Nemonic.CALL:
          writeStream.write(bytecode[i][j].nemonic + " " + bytecode[i][j].bcType.r1 + " " + bytecode[i][j].bcType.n1 + "\n");
          break;

        case Nemonic.SETARG:
        case Nemonic.SETLOCAL:
          writeStream.write(bytecode[i][j].nemonic + " " + bytecode[i][j].bcType.n1 + " " + bytecode[i][j].bcType.n2 + " " + bytecode[i][j].bcType.r1 + "\n");
          break;

        case Nemonic.GETIDX:
          writeStream.write(bytecode[i][j].nemonic + " " + bytecode[i][j].bcType.r1 + " " + bytecode[i][j].bcType.r2 + "\n");
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
      throw new Error(Location.LOC_REGISTER + ": NOT IMPLEMENTED YET.");
      break;
    case Location.LOC_GLOBAL:
      //tn is a register
      var tn = searchUnusedReg();
      setBytecodeString(Nemonic.STRING, 0, tn, str);
      setBytecodeBiReg(Nemonic.SETGLOBAL, 0, tn, src);
      break;
    case Location.LOC_LOCAL:
      setBytecodeVariable(Nemonic.SETLOCAL, 0, level, offset, src);
      break;
    case Location.LOC_ARG:
      setBytecodeVariable(Nemonic.SETARG, 0, level, index, src);
      break;
    default:
      throw new Error(location + ": NOT IMPLEMENTED YET.");
  }
}

function compileFunction(functionTable, index){
  var rho = functionTable.rho;
  currentCode = index;
  currentCodeNum = 0;
  maxFuncFl = 0;
  var retu;
  var existClosure, useArguments;
  existClosure = functionTable.existClosure ||
                  searchFunctionDefinition(functionTable.node.body);
  useArguments = searchUseArguments(functionTable.node.body);
  //TODO: figure out OPT_FRAME in .c file
  existClosure = true;
  setBytecodeUniReg(Nemonic.GETGLOBALOBJ, 0, 1);
  if(existClosure || useArguments){
    setBytecodeUniReg(Nemonic.NEWARGS, 0, 0);
    functionTable.existClosure = true;
  }
  setBytecodeNum(Nemonic.SETFL, 2, 0);
  initRegTbl(functionTable.level);

  //register arguments in rho
  for(var i=0; i<functionTable.node.params.length; i++){
    var name = functionTable.node.params[i].name;
    if(existClosure || useArguments){
      rho = environmentExpand(name, Location.LOC_ARG, functionTable.level, 1, i, rho);
    } else {
      var r = searchUnusedReg();
      rho = environmentExpand(name, Location.LOC_REGISTER, functionTable.level, 1, r, rho);
    }
  }

  var dst = 2;
  for(var i=0; i<functionTable.node.body.body.length; i++){
    if(functionTable.node.body.body[i].type == Tokens.VARIABLEDECLARATION){
      for(var j=0; j<functionTable.node.body.body[i].declarations.length; j++){
        if(functionTable.node.body.body[i].declarations[j].type == Tokens.VARIABLEDECLARATOR){
          var name = functionTable.node.body.body[i].declarations[j].id.name;
          if(existClosure || useArguments){
            rho = environmentExpand(name, Location.LOC_LOCAL, functionTable.level, dst++, 1, rho);
          } else {
            var r = searchUnusedReg();
            rho = environmentExpand(name, Location.LOC_REGISTER, functionTable.level, 1, r, rho);
          }
        }
      }
    }
  }

  variableNum[index] = dst;

  compileBytecode(functionTable.node.body, rho, searchUnusedReg(), 0, functionTable.level);
  retu = searchUnusedReg();
  setBytecodeCons(Nemonic.SPECCONST, 1, retu, Const.UNDEFINED);
  setBytecodeUniReg(Nemonic.SETA, 0, retu);
  setBytecodeUniReg(Nemonic.RET, 0, 0);
}

var compileBytecode = function(root, rho, dst, tailFlag, currentLevel){
  if(root == null){
    return;
  }

  if(TOKENLIST.indexOf(root.type) == undefined){
    throw new Error(root.type + ": NOT IMPLEMENTED YET.");
  }

  switch (root.type) {
    case Tokens.PROGRAM:
      for(var i = 0; i < root.body.length; i++){
        compileBytecode(root.body[i], rho, dst, tailFlag, currentLevel);
      }
      break;

    case Tokens.BINARYEXPRESSION:
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

    case Tokens.ASSIGNMENTEXPRESSION:
      if(root.left.type == Tokens.IDENTIFIER){
        var name = root.left.name;
        var t1 = searchUnusedReg();
        var t2 = searchUnusedReg();
        if(root.operator == "="){
          compileBytecode(root.right, rho, dst, 0, currentLevel);
          compileAssignment(name, rho, dst, currentLevel);
        } else {
          compileBytecode(root.left, rho, t1, 0, currentLevel);
          compileBytecode(root.right, rho, t2, 0, currentLevel);
          setBytecodeTriReg(arithNemonic(root.operator.substring(0,1)), 0, dst, t1, t2);
          compileAssignment(name, rho, dst, currentLevel);
        }
      } else if(root.left.type == Tokens.MEMBEREXPRESSION){
            if((root.left.computed === true) &&
              ((root.left.property.type == Tokens.IDENTIFIER) ||
               (typeof root.left.property.value == "number"))){
              var t1 = searchUnusedReg();
              var t2 = searchUnusedReg();
              var t3 = searchUnusedReg();
              var t4 = searchUnusedReg();
              var t5 = searchUnusedReg();
              var fGetIdX = searchUnusedReg();
              if(root.operator == "="){
                compileBytecode(root.left.object, rho, t1, 0, currentLevel);
                compileBytecode(root.left.property, rho, t3, 0, currentLevel);
                compileBytecode(root.right, rho, t2, 0, currentLevel);
                setBytecodeBiReg(Nemonic.GETIDX, 0, fGetIdX, t3);
                setBytecodeBiReg(Nemonic.MOVE, 2, 0, t3);
                setBytecodeRegnum(Nemonic.SEND, 0, fGetIdX, 0);
                setBytecodeNum(Nemonic.SETFL, 2, 0);
                setBytecodeUniReg(Nemonic.GETA, 0, t4);
                setBytecodeTriReg(Nemonic.SETPROP, 0, t1, t4, t2);
              } else {
                compileBytecode(root.left.object, rho, t1, 0, currentLevel);
                compileBytecode(root.left.property, rho, t3, 0, currentLevel);
                compileBytecode(root.right, rho, t2, 0, currentLevel);
                setBytecodeBiReg(Nemonic.GETIDX, 0, fGetIdX, t3);
                setBytecodeBiReg(Nemonic.MOVE, 2, 0, t3);
                setBytecodeRegnum(Nemonic.SEND, 0, fGetIdX, 0);
                setBytecodeNum(Nemonic.SETFL, 2, 0);
                setBytecodeUniReg(Nemonic.GETA, 0, t4);
                setBytecodeTriReg(Nemonic.GETPROP, 0, t5, t1, t4);
                setBytecodeTriReg(arithNemonic(root.operator.substring(0,1)), 0, t5, t5, t4);
                setBytecodeTriReg(Nemonic.SETPROP, 0, t1, t4, t5);
              }
            } else if(root.left.property.type == Tokens.LITERAL ||
                      root.left.computed === false){
                var t1 = searchUnusedReg()
                var t2 = searchUnusedReg()
                var t3 = searchUnusedReg()
                var t4 = searchUnusedReg()
                var str
                if(root.left.property.type === Tokens.IDENTIFIER)
                  str = root.left.property.name
                else
                  var str = root.left.property.value
                if(root.operator === "="){
                  compileBytecode(root.left.object, rho, t1, 0, currentLevel)
                  compileBytecode(root.right, rho, dst, 0, currentLevel)
                  setBytecodeString(Nemonic.STRING, 0, t3, str)
                  setBytecodeTriReg(Nemonic.SETPROP, 0, t1, t3, dst)
                }
            }
          }
      break;

    case Tokens.VARIABLEDECLARATION:
      for(var i = 0; i < root.declarations.length; i++){
        compileBytecode(root.declarations[i], rho, dst, tailFlag, currentLevel);
      }
      break;

    case Tokens.VARIABLEDECLARATOR:
      if(root.init != null){
        var name = root.id.name;
        compileBytecode(root.init, rho, dst, 0, currentLevel);
        compileAssignment(name, rho, dst, currentLevel);
      }
      break;

    case Tokens.LITERAL:
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
        case "string":
          var str = root.value;
          setBytecodeString(Nemonic.STRING, 0, dst, str);
          break;

        default:
          throw new Error("'" + typeof root.value + "' NOT IMPLEMENTED YET.");
      }
      break;

    case Tokens.THISEXPRESSION:
      setBytecodeBiReg(Nemonic.MOVE, 0, dst, 1);
      break;

    case Tokens.IDENTIFIER:
      var name = root.name;
      var tn = searchUnusedReg();
      var level, offset, index, location;
      var obj = environmentLookup(rho, currentLevel, name);

      level = obj.level;
      offset = obj.offset;
      index = obj.index;
      location = obj.location;

      switch (location) {
        //TODO: Add LOC_LOCAL, LOC_REGISTER, and LOC_ARG
        case Location.LOC_REGISTER:
          setBytecodeBiReg(Nemonic.MOVE, 0, dst, index);
          break;
        case Location.LOC_LOCAL:
          setBytecodeVariable(Nemonic.GETLOCAL, 0, level, offset, dst);
          break;
        case Location.LOC_ARG:
          setBytecodeVariable(Nemonic.GETARG, 0, level, index, dst);
          break;
        case Location.LOC_GLOBAL:
          setBytecodeString(Nemonic.STRING, 0, tn, name);
          setBytecodeBiReg(Nemonic.GETGLOBAL, 0, dst, tn);
          break;
        default:
          throw new Error("Unexpected case in case 'Identifier'");
      }
      break;

    case Tokens.EXPRESSIONSTATEMENT:
      compileBytecode(root.expression, rho, dst, 0, currentLevel);
      break;
    case Tokens.SEQUENCEEXPRESSION:
      for(var i=0; i<root.expressions.length; i++){
        compileBytecode(root.expressions[i], rho, dst, 0, currentLevel);
      }
      break;

    case Tokens.IFSTATEMENT:
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

    case Tokens.BLOCKSTATEMENT:
      for(var i=0; i<root.body.length; i++){
        compileBytecode(root.body[i], rho, dst, tailFlag, currentLevel);
      }
      break;

    case Tokens.WHILESTATEMENT:
      var t = searchUnusedReg();
      var l1 = currentLabel++;
      var l2 = currentLabel++;
      var j1, j2;
      j1 = currentCodeNum;
      setBytecodeNum(Nemonic.JUMP, 1, l1);
      l2 = currentCodeNum;
      compileBytecode(root.body, rho, dst, 0, currentLevel);
      l1 = currentCodeNum;
      compileBytecode(root.test, rho, t, 0, currentLevel);
      j2 = currentCodeNum;
      setBytecodeRegnum(Nemonic.JUMPTRUE, 1, t, l2);
      dispatchLabel(j1, l1);
      dispatchLabel(j2, l2);
      break;

    case Tokens.DOWHILESTATEMENT:
      var t = searchUnusedReg();
      var l1 = currentLabel++;
      var j1;
      l1 = currentCodeNum;
      compileBytecode(root.body, rho, dst, 0, currentLevel);
      compileBytecode(root.test, rho, t, 0, currentLevel);
      j1 = currentCodeNum;
      setBytecodeRegnum(Nemonic.JUMPTRUE, 1, t, l1);
      dispatchLabel(j1, l1);
      break;

    case Tokens.FORSTATEMENT:
      //TODO: add closure part
      var t = searchUnusedReg();
      var l1 = currentLabel++;
      var l2 = currentLabel++;
      var j1, j2;
      compileBytecode(root.init, rho, t, 0, currentLevel);
      j1 = currentCodeNum;
      setBytecodeNum(Nemonic.JUMP, 1, l1);
      l2 = currentCodeNum;
      compileBytecode(root.body, rho, dst, 0, currentLevel);
      compileBytecode(root.update, rho, t, 0, currentLevel);
      l1 = currentCodeNum;
      compileBytecode(root.test, rho, t, 0, currentLevel);
      j2 = currentCodeNum;
      setBytecodeRegnum(Nemonic.JUMPTRUE, 1, t, l2);
      dispatchLabel(j1, l1);
      dispatchLabel(j2, l2);
      break;

    case Tokens.FUNCTIONEXPRESSION:
    case Tokens.FUNCTIONDECLARATION:
      var n = addFunctionTable(root, rho, currentLevel+1);
      setBytecodeRegnum(Nemonic.MAKECLOSURE, 0, dst, n);
      break;

    case Tokens.RETURNSTATEMENT:
      var t = searchUnusedReg();
      compileBytecode(root.argument, rho, t, 1, currentLevel);
      setBytecodeUniReg(Nemonic.SETA, 0, t);
      setBytecodeUniReg(Nemonic.RET, 0, -1);
      break;

    case Tokens.CALLEXPRESSION:
      var t = new Array(root.arguments.length + 1);
      for(var i=0; i<t.length; i++){
        t[i] = searchUnusedReg();
      }

      compileBytecode(root.callee, rho, t[0], 0, currentLevel);
      for(var i=1; i<t.length; i++){
        compileBytecode(root.arguments[i-1], rho, t[i], 0, currentLevel);
      }
      //TODO: tailflag
      for(var i=1; i<t.length; i++){
        setBytecodeBiReg(Nemonic.MOVE, 2, i - root.arguments.length, t[i]);
      }

      setBytecodeRegnum(Nemonic.CALL, 0, t[0], root.arguments.length);
      setBytecodeNum(Nemonic.SETFL, 2, 0);
      setBytecodeUniReg(Nemonic.GETA, 0, dst);

      if(root.arguments.length > maxFuncFl){
        maxFuncFl = root.arguments.length+1;
      }
      break;

    case Tokens.ARRAYEXPRESSION:
      var objConsStr = searchUnusedReg();
      var objCons = searchUnusedReg();
      var propdst = searchUnusedReg();
      var dlen = searchUnusedReg();
      var count = root.elements.length;
      setBytecodeIVal(Nemonic.FIXNUM, 0, dlen, count);
      setBytecodeString(Nemonic.STRING, 0, objConsStr, gStringArray);
      setBytecodeBiReg(Nemonic.GETGLOBAL, 0, objCons, objConsStr);
      setBytecodeBiReg(Nemonic.NEW, 0, dst, objCons);
      setBytecodeBiReg(Nemonic.MOVE, 2, 0, dlen);
      setBytecodeBiReg(Nemonic.MOVE, 2, -1, dst);
      setBytecodeRegnum(Nemonic.NEWSEND, 0, objCons, 1);
      setBytecodeNum(Nemonic.SETFL, 2, 0);
      setBytecodeUniReg(Nemonic.GETA, 0, dst);
      for(var i=0; i<root.elements.length; i++){
        compileBytecode(root.elements[i], rho, propdst, 0, currentLevel);
        setBytecodeTriReg(Nemonic.SETARRAY, 0, dst, i, propdst);
      }
      if(1 > maxFuncFl){
        maxFuncFl = 1;
      }
      break;

    case Tokens.MEMBEREXPRESSION:
      if((root.computed === true) && (typeof root.property.value === "number")){
        var r1 = searchUnusedReg();
        var r2 = searchUnusedReg();
        var fGetIdX = searchUnusedReg();
        var r3 = searchUnusedReg();
        compileBytecode(root.object, rho, r1, 0, currentLevel);
        compileBytecode(root.property, rho, r2, 0, currentLevel);
        setBytecodeBiReg(Nemonic.GETIDX, 0, fGetIdX, r2);
        setBytecodeBiReg(Nemonic.MOVE, 2, 0, r2);
        setBytecodeRegnum(Nemonic.SEND, 0, fGetIdX, 0);
        setBytecodeNum(Nemonic.SETFL, 2, 0);
        setBytecodeUniReg(Nemonic.GETA, 0, r3);
        setBytecodeTriReg(Nemonic.GETPROP, 0, dst, r1, r3);
        if(maxFuncFl === 0) maxFuncFl = 1;
      } else {
        var t1 = searchUnusedReg();
        var t2 = searchUnusedReg();
        var str = root.property.value;
        compileBytecode(root.object, rho, t1, 0, currentLevel);
        setBytecodeString(Nemonic.STRING, 0, t2, str);
        setBytecodeTriReg(Nemonic.GETPROP, 0, dst, t1, t2);
      }
      break;

    case Tokens.OBJECTEXPRESSION:
      var objConsStr = searchUnusedReg();
      var objCons = searchUnusedReg();
      var propDst = searchUnusedReg();
      var pstr = searchUnusedReg();
      var fGetIdX = searchUnusedReg();

      setBytecodeString(Nemonic.STRING, 0, objConsStr, gStringObject);
      setBytecodeBiReg(Nemonic.GETGLOBAL, 0, objCons, objConsStr);
      setBytecodeBiReg(Nemonic.NEW, 0, dst, objCons);
      setBytecodeBiReg(Nemonic.MOVE, 2, 0, dst);
      setBytecodeRegnum(Nemonic.NEWSEND, 0, objCons, 0);
      setBytecodeNum(Nemonic.SETFL, 2, 0);
      setBytecodeUniReg(Nemonic.GETA, 0, dst);

      for(var i=0; i<root.properties.length; i++){
        if(typeof root.properties[i].key.value === "number"){
             compileBytecode(root.properties[i].value, rho, propDst, 0, currentLevel);
             var val = root.properties[i].key.value;
             if(Number.isSafeInteger(val)){
               setBytecodeIVal(Nemonic.FIXNUM, 0, pstr, val);
             } else{
               setBytecodeDVal(Nemonic.NUMBER, 0, pstr, val);
             }
             setBytecodeBiReg(Nemonic.GETIDX, 0, fGetIdX, pstr);
             setBytecodeBiReg(Nemonic.MOVE, 2, 0, pstr);
             setBytecodeRegnum(Nemonic.SEND, 0, fGetIdX, 0);
             setBytecodeNum(Nemonic.SETFL, 2, 0);
             setBytecodeUniReg(Nemonic.GETA, 0, pstr);
             setBytecodeTriReg(Nemonic.SETPROP, 0, dst, pstr, propDst);
           } else {
             var name
             //TOK_NAME
             if(root.properties[i].key.type === Tokens.IDENTIFIER)
                name = root.properties[i].key.name
             //TOK_STRING
             else
                name = root.properties[i].key.value;
             compileBytecode(root.properties[i].value, rho, propDst, 0, currentLevel);
             setBytecodeString(Nemonic.STRING, 0, pstr, name);
             setBytecodeTriReg(Nemonic.SETPROP, 0, dst, pstr, propDst);
           }
      }
      if(root.properties.length > maxFuncFl){
        maxFuncFl = root.properties.length;
      }
      break;

    case Tokens.NEWEXPRESSION:
      var tmp = []
      var ts = searchUnusedReg()
      var tg = searchUnusedReg()
      var tins = searchUnusedReg()
      var retr = searchUnusedReg()
      var l1 = currentLabel++
      var j1
      for(var i=0; i<=root.arguments.length; i++){
        tmp[i] = searchUnusedReg()
      }
      compileBytecode(root.callee, rho, tmp[0], 0, currentLevel)
      for(i=1; i<=root.arguments.length; i++){
        compileBytecode(root.arguments[i-1], rho, tmp[i], 0, currentLevel)
      }
      for(i=0; i<root.arguments.length; i++){
        setBytecodeBiReg(Nemonic.MOVE, 2, -root.arguments.length + i + 1, tmp[i+1])
      }
      setBytecodeBiReg(Nemonic.NEW, 0, dst, tmp[0])
      setBytecodeBiReg(Nemonic.MOVE, 2, -root.arguments.length, dst)
      setBytecodeRegnum(Nemonic.NEWSEND, 0, tmp[0], root.arguments.length)
      setBytecodeNum(Nemonic.SETFL, 2, 0)
      setBytecodeString(Nemonic.STRING, 0, ts, "Object")
      setBytecodeBiReg(Nemonic.GETGLOBAL, 0, tg, ts)
      setBytecodeUniReg(Nemonic.GETA, 0, retr)
      setBytecodeTriReg(Nemonic.INSTANCEOF, 0, tins, retr, tg)
      j1 = currentCodeNum
      setBytecodeRegnum(Nemonic.JUMPFALSE, 1, tins, l1)
      setBytecodeUniReg(Nemonic.GETA, 0, dst)
      l1 = currentCodeNum
      if(root.arguments.length+1 > maxFuncFl){
        maxFuncFl = root.arguments.length + 1
      }

      dispatchLabel(j1, l1)
      break;

    case Tokens.UPDATEEXPRESSION:
      if(root.argument.type === Tokens.IDENTIFIER){
        if(root.prefix){
          var t1 = searchUnusedReg();
          var tone = searchUnusedReg();
          var str = root.argument.name;
          var one = 1;
          compileBytecode(root.argument, rho, t1, 0, currentLevel);
          setBytecodeIVal(Nemonic.FIXNUM, 0, tone, one)
          setBytecodeTriReg(arithNemonic(root.operator.substring(0,1)), 0, dst, t1, tone);
          compileAssignment(str, rho, dst, currentLevel);
        } else {
          var t1 = searchUnusedReg();
          var tone = searchUnusedReg();
          var str = root.argument.name;
          var one = 1;
          compileBytecode(root.argument, rho, dst, 0, currentLevel);
          setBytecodeIVal(Nemonic.FIXNUM, 0, tone, one);
          setBytecodeTriReg(arithNemonic(root.operator.substring(0,1)), 0, t1, dst, tone);
          compileAssignment(str, rho, t1, currentLevel);
        }
      } else if(root.argument.type === Tokens.MEMBEREXPRESSION){
          if(root.prefix){
            var t1 = searchUnusedReg();
            var t2 = searchUnusedReg();
            var tone = searchUnusedReg();
            var tn = searchUnusedReg();
            var str = root.argument.property.name;
            var one = 1;
            compileBytecode(root.argument.object, rho, t2, 0, currentLevel);
            setBytecodeString(Nemonic.STRING, 0, tn, str);
            setBytecodeTriReg(Nemonic.GETPROP, 0, t1, t2, tn);
            setBytecodeIVal(Nemonic.FIXNUM, 0, tone, one);
            setBytecodeTriReg(arithNemonic(root.operator.substring(0,1)), 0, dst, t1, tone);
            setBytecodeTriReg(Nemonic.SETPROP, 0, t2, tn, dst);
          } else {
            var t1 = searchUnusedReg();
            var t2 = searchUnusedReg();
            var tone = searchUnusedReg();
            var tn = searchUnusedReg();
            var str = root.argument.property.name;
            var one = 1;
            compileBytecode(root.argument.object, rho, t2, 0, currentLevel);
            setBytecodeString(Nemonic.STRING, 0, tn, str);
            setBytecodeTriReg(Nemonic.GETPROP, 0, dst, t2, tn);
            setBytecodeIVal(Nemonic.FIXNUM, 0, tone, one);
            setBytecodeTriReg(arithNemonic(root.operator.substring(0,1)), 0, t1, dst, tone);
            setBytecodeTriReg(Nemonic.SETPROP, 0, t2, tn, t1);
          }
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
  initRegTbl(0);
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
  codeNum[0] = currentCodeNum;

  for(var i=1; i<currentFunction; i++){
    compileFunction(functionTable[i], i);
    codeNum[i] = currentCodeNum;
    frameLinkTable[currentCode] = calculateFrameLink();
    setBytecodeFl(currentCode);
  }

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

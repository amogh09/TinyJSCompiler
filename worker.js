var tokens = ["ArrayExpression", "ArrayPattern", "ArrowFunctionExpression",
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

for(var i=0; i<tokens.length; i++){
  var lhs = tokens[i].toUpperCase();
  var rhs = '"' + tokens[i] + '"';
  console.log(lhs + ": " + rhs + ", ");
}

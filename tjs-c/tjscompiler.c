//
// tjscompiler.c
// SSJS project, iwasaki-lab, UEC, Japan
//
// Sho Takada, 2011-2013
// Hideya Iwasaki, 2013
// Ryota Fujii, 2013
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <unistd.h>
#include <inttypes.h>

#include <jsapi.h>
#include <jsparse.h>
#include <jsscan.h>
#include <jsstr.h>
#include <jsatom.h>
#include <jsfun.h>
#include <jsscope.h>
#include <jsinterp.h>
#include <jsregexp.h>

//#define FL_MAX 200 // REGISTER_MAX と同義？
#define FL_MAX 1500 //2011/05/30追記
#define VARIABLE_MAX 100
#define ENVTABLE_MAX 300
#define DEBUG1 0
#define DEBUG2 0
#define DEBUG3 0
#define DEBUG4 0
#define REG_A 9999
#define MAX_BYTECODE 10000
#define F_REGEXP_NONE (0x0)
#define F_REGEXP_GLOBAL (0x1)
#define F_REGEXP_IGNORE (0x2)
#define F_REGEXP_MULTILINE (0x4)

#define release_reg(curr_tbl,n)  //(used_reg_tbl[(n)] = 0) 

/*  2013/08/20  Iwasaki */
#define OUTPUT_SBC    0
#define OUTPUT_SEXPR  1
int output_format = OUTPUT_SBC;

#define FLEN   128
char *output_filename = NULL;
char *input_filename = NULL;
char filename[FLEN];

/*
	関数呼出し後のaddsp を setflに
	breakとcontinueを考えてなかった
	配列にぶち込んでおいてあとでまとめて解決でいいか
	でもそうするとネストはどうするんだ？
	ネストを適当に書いておけばいいか
	グローバル変数でいいか
	ラベル番号どうするか
*/


/* from jsscan.h */
const char * TOKENS[81] = {
  "EOF", "EOL", "SEMI", "COMMA", "ASSIGN", "HOOK", "COLON", "OR", "AND",
  "BITOR", "BITXOR", "BITAND", "EQOP", "RELOP", "SHOP", "PLUS", "MINUS", "STAR",
  "DIVOP", "UNARYOP", "INC", "DEC", "DOT", "LB", "RB", "LC", "RC", "LP", "RP",
  "NAME", "NUMBER", "STRING", "OBJECT", "PRIMARY", "FUNCTION", "EXPORT",
  "IMPORT", "IF", "ELSE", "SWITCH", "CASE", "DEFAULT", "WHILE", "DO", "FOR",
  "BREAK", "CONTINUE", "IN", "VAR", "WITH", "RETURN", "NEW", "DELETE",
  "DEFSHARP", "USESHARP", "TRY", "CATCH", "FINALLY", "THROW", "INSTANCEOF",
  "DEBUGGER", "XMLSTAGO", "XMLETAGO", "XMLPTAGC", "XMLTAGC", "XMLNAME",
  "XMLATTR", "XMLSPACE", "XMLTEXT", "XMLCOMMENT", "XMLCDATA", "XMLPI", "AT",
  "DBLCOLON", "ANYNAME", "DBLDOT", "FILTER", "XMLELEM", "XMLLIST", "RESERVED",
  "LIMIT",
};

const int NUM_TOKENS = sizeof(TOKENS) / sizeof(TOKENS[0]);
JSContext * context;
typedef int regexpFlag;

char *gStringObject = "Object";
char *gStringArray = "Array";

typedef enum {
  LOC_ARGUMENTS,
  LOC_REGISTER,
  LOC_CATCH,
  LOC_LOCAL,
  LOC_ARG,
  LOC_GLOBAL
} Location;

typedef enum {
  NUMBER,
  FIXNUM,
  STRING,
  REGEXP,
  CONST,
  SPECCONST,
  ADD,
  SUB,
  MUL,
  DIV,
  MOD,
  BITAND,
  BITOR,
  JUMPTRUE,
  JUMPFALSE,
  JUMP,
  LEFTSHIFT,
  RIGHTSHIFT,
  UNSIGNEDRIGHTSHIFT,
  LESSTHAN,
  LESSTHANEQUAL,
  EQ,
  EQUAL,
  GETPROP,
  SETPROP,
  SETARRAY,
  GETARG,
  SETARG,
  GETLOCAL,
  SETLOCAL,
  GETGLOBAL,
  SETGLOBAL,
  MOVE,
  CALL,
  SEND,
  TAILCALL,
  TAILSEND,
  NEWSEND,
  RET,
  MAKECLOSURE,
  MAKEITERATOR,
  NEXTPROPNAME,
  TRY,
  THROW,
  FINALLY,
  NEW,
  INSTANCEOF,
  TYPEOF,
  NOT,
  NEWARGS,
  SETFL,
  ADDSP,
  SUBSP,
  GETIDX, // 2011/05/30追記
  ISUNDEF, 
  ISOBJECT,
  SETA,
  GETA,
  GETERR,
  GETGLOBALOBJ,
  ERROR,
  UNKNOWN
} Nemonic;

typedef enum {
  TRUE,
  FALSE,
  UNDEFINED,
  Null
} Const;

typedef struct environment {
  char *name;
  Location location;
  int level;
  int offset;
  int index;
  struct environment* next;
} *Environment;

typedef int Register;

typedef struct function_tbl{
  JSParseNode *node;
  Environment rho;
  bool existClosure;
  int level;
  int call_entry;
  int send_entry;
} Function_tbl;

typedef struct bytecope {
  Nemonic nemonic;
  int label;
  int flag;
  int call_entry;
  int send_entry;
  union {
    struct {
      Register dst;
      double val;
    } dval;
    struct {
      Register dst;
      int64_t val;
    } ival;
    struct {
      Register dst;
      char *str;
    } str;
    struct {
      Register dst;
      char *str;
      regexpFlag flag;
    } regexp;
    struct {
      Register dst;
      Const cons;
    } cons;
    struct {
      int n1;
    } num;
    struct {
      int n1;
      int n2;
      Register r1;
    } var;
    struct {
      Register r1;
      int n1;
    } regnum;
    struct {
      Register r1;
    } unireg;
    struct {
      Register r1;
      Register r2;
    } bireg;
    struct {
      Register r1;
      Register r2;
      Register r3;
    } trireg;
  } bc_u;
} Bytecode;

char *regexp_to_string(JSAtom *atom, regexpFlag *flag);


char * atom_to_string(JSAtom *atom);

int fl=100;
int curr_tbl=0;
int used_reg_tbl[FL_MAX]; //2011/05/30追記
//int used_reg_tbl[200];
int touch_reg_tbl[FL_MAX]; //2011/05/30追記
//int touch_reg_tbl[200];
int fl_tbl[201];
Function_tbl func_tbl[201] = {};
int var_num[201];
int curr_func = 1;
int code_num[201];
int curr_label = 1;
int curr_code_num = 0;
int curr_code = 0;
Bytecode bytecode[201][MAX_BYTECODE];
int max_func_fl;

bool searchFunctionDefinition(JSParseNode *root) {
  if (root == NULL || root == (JSParseNode *)0xffffffff ||
      root == (JSParseNode*)0xdadadadadadadada) {
    return false;
  }
  if (root->pn_type >= NUM_TOKENS) {
    return false;
  }
  switch (root->pn_type) {
  case TOK_PLUS:
  case TOK_MINUS:
  case TOK_STAR:
  case TOK_DIVOP:
  case TOK_BITAND:
  case TOK_BITOR:
    {
      if (root->pn_arity == PN_BINARY) {
        return searchFunctionDefinition(root->pn_left) ||
               searchFunctionDefinition(root->pn_right);
      } else if (root->pn_arity == PN_LIST) {
        bool f = false;
        JSParseNode *p;
        f = searchFunctionDefinition(root->pn_head);
        for (p = root->pn_head->pn_next; p != NULL && !f; p = p->pn_next) {
          f = searchFunctionDefinition(p);
        }
        return f;
      } else {
        return false;
      }
    }
  case TOK_ASSIGN:
    if (root->pn_left->pn_type == TOK_LB) {
      return searchFunctionDefinition(root->pn_left) ||
             searchFunctionDefinition(root->pn_right);
    } else {
      return searchFunctionDefinition(root->pn_right);
    }
  case TOK_DOT:
    return searchFunctionDefinition(root->pn_expr);
  case TOK_LB:
  case TOK_WHILE:
  case TOK_DO:
    return searchFunctionDefinition(root->pn_left) ||
           searchFunctionDefinition(root->pn_right);
  case TOK_UNARYOP:
  case TOK_RETURN:
    return searchFunctionDefinition(root->pn_kid);
  case TOK_AND:
  case TOK_OR:
  case TOK_RELOP:
  case TOK_EQOP:
    return searchFunctionDefinition(root->pn_left) ||
           searchFunctionDefinition(root->pn_right);
  case TOK_HOOK:
  case TOK_IF:
    return searchFunctionDefinition(root->pn_kid1) ||
           searchFunctionDefinition(root->pn_kid2) ||
           searchFunctionDefinition(root->pn_kid3);
  case TOK_FOR:
    if (root->pn_left->pn_arity == PN_TERNARY) {
      return searchFunctionDefinition(root->pn_left->pn_kid1) ||
             searchFunctionDefinition(root->pn_left->pn_kid2) ||
             searchFunctionDefinition(root->pn_left->pn_kid3) ||
             searchFunctionDefinition(root->pn_right);
    } else {
      return searchFunctionDefinition(root->pn_left->pn_right) ||
             searchFunctionDefinition(root->pn_right);
    }
  case TOK_LP:
    {
      bool f = false;
      JSParseNode *p;
      if (root->pn_head->pn_type == TOK_DOT) {
        f = searchFunctionDefinition(root->pn_head->pn_expr);
      } else if (root->pn_head->pn_type == TOK_LB) {
        f = searchFunctionDefinition(root->pn_head->pn_left) ||
            searchFunctionDefinition(root->pn_head->pn_right);
      }
      for (p = root->pn_head->pn_next; p != NULL && !f; p = p->pn_next) {
        f = searchFunctionDefinition(p);
      }
      return f;
    }
  case TOK_NEW:
    {
      bool f;
      JSParseNode *p;
      for (p = root->pn_head, f = false; p != NULL && !f; p = p->pn_next) {
        f = searchFunctionDefinition(p);
      }
      return f;
    }
  case TOK_VAR:
    {
      bool f;
      JSParseNode *p;
      for (p = root->pn_head, f = false; p != NULL && !f; p = p->pn_next) {
        if (p->pn_expr != NULL) {
          f = searchFunctionDefinition(p->pn_expr);
        }
      }
      return f;
    }
    /*TOK_NEWから実装を再開すること！*/
  case TOK_FUNCTION:
    return true;
  case TOK_NUMBER:
  case TOK_STRING:
  case TOK_PRIMARY:
  case TOK_NAME:
  case TOK_INC:
  case TOK_DEC:
    return false;
  default:
    switch (root->pn_arity) {
    case PN_UNARY:
      return searchFunctionDefinition(root->pn_kid);
    case PN_BINARY:
      return searchFunctionDefinition(root->pn_left) ||
             searchFunctionDefinition(root->pn_right);
    case PN_TERNARY:
      return searchFunctionDefinition(root->pn_kid1) ||
             searchFunctionDefinition(root->pn_kid2) ||
             searchFunctionDefinition(root->pn_kid3);
    case PN_LIST:
      {
        bool f;
        JSParseNode *p;
        for (p = root->pn_head, f = false; p != NULL && !f; p = p->pn_next) {
          f = searchFunctionDefinition(p);
        }
        return f;
      }
    case PN_NAME:
      if (root->pn_expr != NULL) {
        return searchFunctionDefinition(root->pn_expr);
      }
    case PN_FUNC:
      return true;
    case PN_NULLARY:
    default:
      return false;
    }				
  }
}
 
bool searchUseArguments(JSParseNode *root)
{
  if (root == NULL) {
    return false;
  }
  if (root->pn_type >= NUM_TOKENS) {
    return false;
  }
  switch (root->pn_type) {
  case TOK_DOT:
      return searchUseArguments(root->pn_expr);
  case TOK_FUNCTION:
    return false;
  case TOK_NUMBER:
  case TOK_STRING:
  case TOK_PRIMARY:
    return false;
  case TOK_NAME:
    {
      char *str = atom_to_string(root->pn_atom);
      // fprintf(stderr, "searchUseArguments:TOK_NAME:ENTER\n");
      // fprintf(stderr, "searchUseArguments:TOK_NAME:atom=%s\n", str);
      if (strcmp(str, "arguments") == 0) {
        return true;
      } else {
        return false;
      }
    }
  default:
    switch (root->pn_arity) {
    case PN_UNARY:
      return searchUseArguments(root->pn_kid);
    case PN_BINARY:
      return searchUseArguments(root->pn_left) ||
             searchUseArguments(root->pn_right);
    case PN_TERNARY:
      return searchUseArguments(root->pn_kid1) ||
             searchUseArguments(root->pn_kid2) ||
             searchUseArguments(root->pn_kid3);
    case PN_LIST:
      {
        bool f;
        JSParseNode *p;
        for (p = root->pn_head, f = false; p != NULL && !f; p = p->pn_next) {
          f = searchUseArguments(p);
        }
        return f;
      }
    case PN_NAME:
      if (root->pn_expr != NULL) {
        return searchUseArguments(root->pn_expr);
      }
    case PN_FUNC:
      return false;
    case PN_NULLARY:
    default:
      return false;
    }				
  }
}

void set_bc_ival(Nemonic nemonic, int flag, Register dst, int64_t ival){
  bytecode[curr_code][curr_code_num].nemonic = nemonic;
  bytecode[curr_code][curr_code_num].flag = flag;
  bytecode[curr_code][curr_code_num].bc_u.ival.dst = dst;
  bytecode[curr_code][curr_code_num++].bc_u.ival.val = ival;
}
// op:NUMBER only
// dst レジスタに dval を格納
// という命令を刻む
void set_bc_dval(Nemonic nemonic, int flag, Register dst, double dval){
  bytecode[curr_code][curr_code_num].nemonic = nemonic;
  bytecode[curr_code][curr_code_num].flag = flag;
  bytecode[curr_code][curr_code_num].bc_u.dval.dst = dst;
  bytecode[curr_code][curr_code_num++].bc_u.dval.val = dval;
}

// op:STRING only
// dst レジスタに str のアドレスを格納
// という命令を刻む
void set_bc_str(Nemonic nemonic, int flag, Register dst, char *str){
  bytecode[curr_code][curr_code_num].nemonic = nemonic;
  bytecode[curr_code][curr_code_num].flag = flag;
  bytecode[curr_code][curr_code_num].bc_u.str.dst = dst;
  bytecode[curr_code][curr_code_num++].bc_u.str.str = str;
}

void set_bc_regexp(Nemonic nemonic, int flag, Register dst, char* str, regexpFlag regflag){
  bytecode[curr_code][curr_code_num].nemonic = nemonic;
  bytecode[curr_code][curr_code_num].flag = flag;
  bytecode[curr_code][curr_code_num].bc_u.regexp.dst = dst;
  bytecode[curr_code][curr_code_num].bc_u.regexp.str = str;
  bytecode[curr_code][curr_code_num++].bc_u.regexp.flag = regflag;
}

// op:CONST only
// dst レジスタに TRUE, FALUSE, UNDEFINED, NULL のいずれかを格納
// という命令を刻む
void set_bc_cons(Nemonic nemonic, int flag, Register dst, Const cons){
  bytecode[curr_code][curr_code_num].nemonic = nemonic;
  bytecode[curr_code][curr_code_num].flag = flag;
  bytecode[curr_code][curr_code_num].bc_u.cons.dst = dst;
  bytecode[curr_code][curr_code_num++].bc_u.cons.cons = cons;
}

// op:SETFL, JUMP
// FL を num にセット, pc+num へジャンプ
// のいずれかの命令を刻む
void set_bc_num(Nemonic nemonic, int flag, int num){
  bytecode[curr_code][curr_code_num].nemonic = nemonic;
  bytecode[curr_code][curr_code_num].flag = flag;
  bytecode[curr_code][curr_code_num++].bc_u.num.n1 = num;
}

// op:JUMPTRUE, JUMPFALSE, SEND, TAILSEND, CALL, TAILCALL, MAKECLOSURE
// dst が TRUE なら label:n1 へジャンプ
// dst が FALSE なら label:n1 へジャンプ
// args.length == n1 な関数 r1 のレシーバのある呼び出し
// args.length == n1 な関数 r1 のレシーバのある末尾呼び出し
// args.length == n1 な関数 r1 のレシーバのない呼び出し
// args.length == n1 な関数 r1 のレシーバのない末尾呼び出し
// func_tbl[n1] の関数クロージャを dst に格納
// のいずれかの命令を刻む
void set_bc_regnum(Nemonic nemonic, int flag, Register r1, int n1){
  bytecode[curr_code][curr_code_num].nemonic = nemonic;
  bytecode[curr_code][curr_code_num].flag = flag;
  bytecode[curr_code][curr_code_num].bc_u.regnum.r1 = r1;
  bytecode[curr_code][curr_code_num++].bc_u.regnum.n1 = n1;
}

// op:RET , SETA, GETA
//
void set_bc_unireg(Nemonic nemonic, int flag, Register r1){
  bytecode[curr_code][curr_code_num].nemonic = nemonic;
  bytecode[curr_code][curr_code_num].flag = flag;
  bytecode[curr_code][curr_code_num++].bc_u.unireg.r1 = r1;
}

// op:GETGLOBAL, SETGLOBAL, TYPEOF, MOVE, NEW, NEWARGS
void set_bc_bireg(Nemonic nemonic, int flag, Register r1, Register r2){
  bytecode[curr_code][curr_code_num].nemonic = nemonic;
  bytecode[curr_code][curr_code_num].flag = flag;
  bytecode[curr_code][curr_code_num].bc_u.bireg.r1 = r1;
  bytecode[curr_code][curr_code_num++].bc_u.bireg.r2 = r2;
}

// op:ARITHMATIC, SETPROP, GETPROP
void set_bc_trireg(Nemonic nemonic, int flag, Register r1, Register r2, Register r3){
  bytecode[curr_code][curr_code_num].nemonic = nemonic;
  bytecode[curr_code][curr_code_num].flag = flag;
  bytecode[curr_code][curr_code_num].bc_u.trireg.r1 = r1;
  bytecode[curr_code][curr_code_num].bc_u.trireg.r2 = r2;
  bytecode[curr_code][curr_code_num++].bc_u.trireg.r3 = r3;
}

// op:SETLOCAL, SETARG, GETLOCAL, GETARG
void set_bc_var(Nemonic nemonic, int flag, int n1, int n2, Register r1){
  bytecode[curr_code][curr_code_num].nemonic = nemonic;
  bytecode[curr_code][curr_code_num].flag = flag;
  bytecode[curr_code][curr_code_num].bc_u.var.n1 = n1;
  bytecode[curr_code][curr_code_num].bc_u.var.n2 = n2;
  bytecode[curr_code][curr_code_num++].bc_u.var.r1 = r1;
}
void set_label(int label)
{
  bytecode[curr_code][curr_code_num].label = label;
}

Environment env_empty(void)
{
  return NULL;
}

Environment env_expand(char *name, Location loc, int level, int offset, int index, Environment rho)
{
  Environment e = malloc(sizeof(struct environment));
  e->name = name;
  e->location = loc;
  e->level = level;
  e->offset = offset;
  e->index = index;
  e->next = rho;
  return e;
}


int search_unuse()
{
  int i;
  for(i=0; used_reg_tbl[i] && i<FL_MAX; i++);
  used_reg_tbl[i] = 1;
  touch_reg_tbl[i] = 1;
  return i;
}

void init_reg_tbl(int curr_level)
{
  int i;
  for (i = 0; i < FL_MAX; i++) {
    release_reg(curr_tbl,i);
    used_reg_tbl[i] = 0; // 2011/05/31追記 release_reg の代わりに解放
    touch_reg_tbl[i] = 0;
  }
  used_reg_tbl[0] = 1;
  touch_reg_tbl[0] = 1;
  used_reg_tbl[1] = 1;
  touch_reg_tbl[1] = 1;
}


Location env_lookup(Environment rho, int curr_level, char *name, int *level, int *offset, int *index)
{
  while (rho != NULL){
    if (strcmp(name, rho->name) == 0) {
      *level = curr_level - rho->level;
      *offset = rho->offset;
      *index = rho->index;
      return rho->location;
    }
    rho = rho->next;
  }
  return LOC_GLOBAL;
}

char *nemonic_to_str(Nemonic nemonic)
{
  switch (nemonic) {
  case NUMBER: return "number";
  case FIXNUM: return "fixnum";
  case STRING: return "string";
  case REGEXP: return "regexp";
  case CONST: return "const";
  case SPECCONST: return "specconst";
  case ADD: return "add";
  case SUB: return "sub";
  case MUL: return "mul";
  case DIV: return "div";
  case MOD: return "mod";
  case BITAND: return "bitand";
  case BITOR: return "bitor";
  case JUMPTRUE: return "jumptrue";
  case JUMPFALSE: return "jumpfalse";
  case JUMP: return "jump";
  case LEFTSHIFT: return "leftshift";
  case RIGHTSHIFT: return "rightshift";
  case UNSIGNEDRIGHTSHIFT: return "unsignedrightshift";
  case LESSTHAN: return "lessthan";
  case LESSTHANEQUAL: return "lessthanequal";
  case EQ: return "eq";
  case EQUAL: return "equal";
  case GETPROP: return "getprop";
  case SETPROP: return "setprop";
  case SETARRAY: return "setarray";
  case GETARG: return "getarg";
  case SETARG: return "setarg";
  case GETLOCAL: return "getlocal";
  case SETLOCAL: return "setlocal";
  case GETGLOBAL: return "getglobal";
  case SETGLOBAL: return "setglobal";
  case MOVE: return "move";
  case CALL: return "call";
  case SEND: return "send";
  case TAILCALL: return "tailcall";
  case TAILSEND: return "tailsend";
  case NEWSEND: return "newsend";
  case RET: return "ret";
  case MAKECLOSURE: return "makeclosure";
  case MAKEITERATOR: return "makeiterator";
  case NEXTPROPNAME: return "nextpropname";
  case TRY: return "try";
  case THROW: return "throw";
  case FINALLY: return "finally";
  case NEW: return "new";
  case INSTANCEOF: return "instanceof";
  case TYPEOF: return "typeof";
  case NOT: return "not";
  case NEWARGS: return "newargs";
  case SETFL: return "setfl";
  case ADDSP: return "addsp";
  case SUBSP: return "subsp";
  case GETIDX: return "getidx"; // 2011/05/30追記
  case ISUNDEF: return "isundef";
  case ISOBJECT: return "isobject";
  case SETA: return "seta";
  case GETA: return "geta";
  case GETERR: return "geterr";
  case GETGLOBALOBJ: return "getglobalobj";
  case ERROR: return "error";
  case UNKNOWN: return "unknown";
  default: return "nemonic_to_str???";
  }
}

char *const_to_str(Const cons){
  switch (cons) {
  case TRUE:      return "true";
  case FALSE:     return "false";
  case UNDEFINED: return "undefined";
  case Null:      return "null";
  default:        return "const_to_str???";
  }
}

Nemonic arith_nemonic(int type)
{
  switch (type) {
  case TOK_PLUS:   return ADD;
  case TOK_MINUS:  return SUB;
  case TOK_STAR:   return MUL;
  case TOK_DIVOP:  return DIV;
  default:         return UNKNOWN;
  }
}
Nemonic shift_nemonic(int type)
{
  switch (type) {
  case JSOP_LSH:  return LEFTSHIFT;
  case JSOP_RSH:  return RIGHTSHIFT;
  case JSOP_URSH: return UNSIGNEDRIGHTSHIFT;
  default: return UNKNOWN;
  }
}

Nemonic divop_nemonic(int type)
{
  switch (type) {
  case 30:  return DIV;
  case 31:  return MOD;
  default:  return UNKNOWN;
  }
}

Nemonic arith_nemonic_prefix(int type)
{
  switch (type) {
  case TOK_INC:  return ADD;
  case TOK_DEC:  return SUB;
  default:       return UNKNOWN;
  }
}

Nemonic arith_nemonic_assignment(int type)
{
  switch (type) {
  case JSOP_LSH: return LEFTSHIFT;
  case JSOP_RSH: return RIGHTSHIFT;
  case JSOP_URSH: return UNSIGNEDRIGHTSHIFT;
  case 15:  return BITOR;
  case 17:  return BITAND;
  case 27:  return ADD;
  case 28:  return SUB;
  case 29:  return MUL;
  case 30:  return DIV;
  case 31:  return MOD;
  default:  return UNKNOWN;
  }
}

Nemonic bitwise_nemonic(int type)
{
  switch (type) {
  case TOK_BITAND: return BITAND;
  case TOK_BITOR: return BITOR;
  default: return UNKNOWN;
  }
}

regexpFlag getFlag(char* flagStart, char* flagEnd)
{
  regexpFlag flag = F_REGEXP_NONE;
  while(flagStart <= flagEnd){
    switch(*flagStart){
    case 'g': flag |= F_REGEXP_GLOBAL; break;
    case 'i': flag |= F_REGEXP_IGNORE; break;
    case 'm': flag |= F_REGEXP_MULTILINE; break;
    }
    flagStart++;
  }
  return flag;	
}

char *regexp_to_string(JSAtom *atom, regexpFlag *flag)
{
  jsval val;
  int length;
  JSString *string;
  char *str, *start, *end, *trueend;
  int i;

  js_regexp_toString(context, ATOM_TO_OBJECT(atom), 0, NULL, &val);
  string = JSVAL_TO_STRING(val);
  length = string->length;
  str = malloc(sizeof(char) * (length + 1));
  for (i=0;i < length ;i++) {
    str[i] = string->chars[i];
  }
  str[i] = '\0';
  start = str + 1;
  trueend = end = str + length;
  while (*end != '/' && start <= end) {
    end--;
  }
  if (*end == '/') {
    *end = '\0';
    end--;
  }
  // fprintf(stderr, "rts:%s, %c\n", start, *end);
  *flag = getFlag(end + 2, trueend);
  return start;
}

char * atom_to_string(JSAtom *atom)
{
  JSString * strings = ATOM_TO_STRING(atom);
  int i;
  char * str;
  int length = strings->length;
  str = malloc(sizeof(char) * (length + 1));
  for(i=0;i < length ;i++){
    str[i] = strings->chars[i];
  }
  str[i]='\0';
  return str;
}

void compile_assignment(char *str, Environment rho, Register src, int curr_level)
{
  int level, offset, index;
  switch (env_lookup(rho, curr_level, str, &level, &offset, &index)) {
  case LOC_REGISTER:
    {
      set_bc_bireg(MOVE, 0, index, src);
    }
    break;
  case LOC_LOCAL:
    {
      set_bc_var(SETLOCAL, 0, level, offset, src);
      //      printf("setlocal %d %d $%d\n", level, offset, src);
    }
    break;
  case LOC_ARG:
    {
      set_bc_var(SETARG, 0, level, index, src);
      //      printf("setarg %d %d $%d\n", level, index, src);
    }
    break;
  case LOC_GLOBAL:
    {
      Register tn = search_unuse();
      set_bc_str(STRING, 0, tn, str);
      set_bc_bireg(SETGLOBAL, 0, tn, src);
      //      printf("string $%d \"%s\"\n", tn, str);
      //      printf("setglobal $%d $%d\n", tn, src);
      release_reg(curr_tbl, tn);
    }
    break;
  default:
    fprintf(stderr, "compile_assignment: unexpected case\n");
  }  
}

int highest_touch_reg() 
{
  int i;
  for (i = 0; touch_reg_tbl[i]; i++);
  return i - 1;
}

int add_function_tbl(JSParseNode *node, Environment rho, int level){
  func_tbl[curr_func].node = node;
  func_tbl[curr_func].rho = rho;
  func_tbl[curr_func].level = level;
  func_tbl[curr_func].existClosure = level > 1;
  return curr_func++ - 1;
}

int calc_fl()
{
//なんだかバグがあるので関数呼び出しがなくても
//関数呼び出し用のレジスタを確保する仕様にしておく
//  if (max_func_fl != 0) {
	return highest_touch_reg() + max_func_fl + 4;
//  } else {
//    return highest_touch_reg();
//  }
}

void set_bc_fl()
{
  int i;
  for (i = 0; i < curr_code_num; i++) {
    if (bytecode[curr_code][i].flag == 2) {
      switch (bytecode[curr_code][i].nemonic) {
      case MOVE:
        bytecode[curr_code][i].flag = 0;
        bytecode[curr_code][i].bc_u.bireg.r1 += fl_tbl[curr_code];
        break;
      case SETFL:
        bytecode[curr_code][i].flag = 0;
        bytecode[curr_code][i].bc_u.num.n1 += fl_tbl[curr_code];
        break;
      default:
        fprintf(stderr, "set_bc_fl: unexpeted instruction %s\n",
                nemonic_to_str(bytecode[curr_code][i].nemonic));
      }
    }
  }
}

void dispatch_label(int jump_line, int label_line)
{
  switch (bytecode[curr_code][jump_line].nemonic) {
  case JUMPTRUE:
  case JUMPFALSE:
    bytecode[curr_code][jump_line].flag = 0;
    bytecode[curr_code][jump_line].bc_u.regnum.n1 = label_line - jump_line;
    break;
  case JUMP:
	case TRY:
    bytecode[curr_code][jump_line].flag = 0;
    bytecode[curr_code][jump_line].bc_u.num.n1 =  label_line - jump_line;
    break;
  default:
    fprintf(stderr, "dispatch_label: unexpeted instruction %s\n",
            nemonic_to_str(bytecode[curr_code][jump_line].nemonic));
    // perror("dispatch failed!");
  }
}

void compile_bytecode(JSParseNode *root, Environment rho, Register dst, int tailflag, int curr_level) 
{
  if (root == NULL) {
    return;
  }
  if (root->pn_type >= NUM_TOKENS) {
    return;
  }
  switch (root->pn_type) {

  case TOK_NUMBER:
    {
      double v = root->pn_dval;
      if(v == (double)(long long)v && v < (double)0xffffffff &&
         v > (double) -0x100000000) {
        set_bc_ival(FIXNUM, 0, dst, (int64_t)v);
      } else {
        set_bc_dval(NUMBER, 0, dst, v);
      }
#if DEBUG4
      printf("dval = %lf\n",
             bytecode[curr_code][curr_code_num-1].bc_u.dval.val);
#endif
      //	printf("number $%d %lf\n", dst, v);
    }
    break;

  case TOK_OBJECT:
    {
      if (root->pn_op == JSOP_REGEXP) {
        char *str;
        regexpFlag flag;
        str = regexp_to_string(root->pn_atom, &flag);
        set_bc_regexp(REGEXP, 0, dst, str, flag);
      }
    }
    break;

  case TOK_SHOP:
    {
      if (root->pn_arity == PN_BINARY) {
        Register t1 = search_unuse();
        Register t2 = search_unuse();
        compile_bytecode(root->pn_left, rho, t1, 0, curr_level);
        compile_bytecode(root->pn_right, rho, t2, 0, curr_level);
        set_bc_trireg(shift_nemonic(root->pn_op), 0, dst, t1, t2);
        // fprintf(stderr, "%d:%d %d %d\n", shift_nemonic(root->pn_op), dst, t1, t2);
        // printf("%s $%d $%d $%d\n", arith_nemonic(root->pn_type), dst, t1, t2);
        release_reg(curr_tbl, t1); 
        release_reg(curr_tbl, t2);
      } else if (root->pn_arity == PN_LIST) {
        JSParseNode * p;
        Register tmp;
        compile_bytecode(root->pn_head, rho, dst, 0, curr_level);
        for (p = root->pn_head->pn_next; p != NULL; p = p->pn_next) {
          tmp = search_unuse();
          compile_bytecode(p, rho, tmp, 0, curr_level);
          set_bc_trireg(shift_nemonic(root->pn_op), 0, dst, dst, tmp);
          // printf("%s $%d $%d $%d\n", arith_nemonic(root->pn_type), dst, dst, tmp);
          release_reg(curr_tbl, tmp);
        }
      }
    }
    break;

  case TOK_PLUS:
  case TOK_MINUS:
  case TOK_STAR:
    {
      if (root->pn_arity == PN_BINARY) {
        Register t1 = search_unuse();
        Register t2 = search_unuse();
        compile_bytecode(root->pn_left, rho, t1, 0, curr_level);
        compile_bytecode(root->pn_right, rho, t2, 0, curr_level);
        set_bc_trireg(arith_nemonic(root->pn_type), 0, dst, t1, t2);
        // printf("%s $%d $%d $%d\n", arith_nemonic(root->pn_type), dst, t1, t2);
        release_reg(curr_tbl, t1); 
        release_reg(curr_tbl, t2);
      } else if (root->pn_arity == PN_LIST) {
        JSParseNode * p;
        Register tmp;
        compile_bytecode(root->pn_head, rho, dst, 0, curr_level);
        for (p = root->pn_head->pn_next; p != NULL; p = p->pn_next) {
          tmp = search_unuse();
          compile_bytecode(p, rho, tmp, 0, curr_level);
          set_bc_trireg(arith_nemonic(root->pn_type), 0, dst, dst, tmp);
          // printf("%s $%d $%d $%d\n",arith_nemonic(root->pn_type), dst, dst, tmp);
          release_reg(curr_tbl, tmp);
        }
      }
    }
    break;

  case TOK_DIVOP:
    {
      if (root->pn_arity == PN_BINARY) {
        Register t1 = search_unuse();
        Register t2 = search_unuse();
        compile_bytecode(root->pn_left, rho, t1, 0, curr_level);
        compile_bytecode(root->pn_right, rho, t2, 0, curr_level);
        set_bc_trireg(divop_nemonic(root->pn_op), 0, dst, t1, t2);
        // printf("%s $%d $%d $%d\n", arith_nemonic(root->pn_type), dst, t1, t2);
        release_reg(curr_tbl, t1); 
        release_reg(curr_tbl, t2);
      } else if (root->pn_arity == PN_LIST) {
        JSParseNode * p;
        Register tmp;
        compile_bytecode(root->pn_head, rho, dst, 0, curr_level);
        for (p = root->pn_head->pn_next; p != NULL; p = p->pn_next) {
          tmp = search_unuse();
          compile_bytecode(p, rho, tmp, 0, curr_level);
          set_bc_trireg(divop_nemonic(root->pn_op), 0, dst, dst, tmp);
          // printf("%s $%d $%d $%d\n",arith_nemonic(root->pn_type), dst, dst, tmp);
          release_reg(curr_tbl, tmp);
        }
      }      
    }
    break;

  case TOK_BITAND:
  case TOK_BITOR:
    {
      if (root->pn_arity == PN_BINARY) {
        Register t1 = search_unuse();
        Register t2 = search_unuse();
        compile_bytecode(root->pn_left, rho, t1, 0, curr_level);
        compile_bytecode(root->pn_right, rho, t2, 0, curr_level);
        set_bc_trireg(bitwise_nemonic(root->pn_type), 0, dst, t1, t2);
        // printf("%s $%d $%d $%d\n", arith_nemonic(root->pn_type), dst, t1, t2);
        release_reg(curr_tbl, t1); 
        release_reg(curr_tbl, t2);
      } else if (root->pn_arity == PN_LIST) {
        JSParseNode * p;
        Register tmp;
        compile_bytecode(root->pn_head, rho, dst, 0, curr_level);
        for (p = root->pn_head->pn_next; p != NULL; p = p->pn_next) {
          tmp = search_unuse();
          compile_bytecode(p, rho, tmp, 0, curr_level);
          set_bc_trireg(bitwise_nemonic(root->pn_type), 0, dst, dst, tmp);
          // printf("%s $%d $%d $%d\n",arith_nemonic(root->pn_type), dst, dst, tmp);
          release_reg(curr_tbl, tmp);
        }
      }      
    }
    break;

  case TOK_STRING:
    {
      char *str = atom_to_string(root->pn_atom);
      set_bc_str(STRING, 0, dst, str);
      // printf("string $%d \"%s\"\n", dst, str);
    }
    break;

  case TOK_PRIMARY:
    {
      int opcode = root->pn_op;
      switch (opcode) {
      case 64:
        set_bc_cons(SPECCONST, 0, dst, Null);
        break;
      case 65:
        set_bc_bireg(MOVE, 0, dst, 1);
        break;
      case 66: /*false*/
        set_bc_cons(SPECCONST, 0, dst, FALSE);
        // printf("const %d %s\n", dst,"False");
        break;
      case 67: /*true*/
        set_bc_cons(SPECCONST, 0, dst, TRUE);
        // printf("const %d %s\n", dst,"True");
        break;
      }
    }
    break;

  case TOK_NAME:
    {
      char *str = atom_to_string(root->pn_atom);	
      int level, offset, index;
      Register tn = search_unuse();
      switch (env_lookup(rho, curr_level, str, &level, &offset, &index)) {
      case LOC_REGISTER:
        set_bc_bireg(MOVE, 0, dst, index);
        break;
      case LOC_LOCAL:
        set_bc_var(GETLOCAL, 0, level, offset, dst);
        // printf("getlocal $%d %d %d\n", dst, level, offset);
        break;
      case LOC_ARG:
        set_bc_var(GETARG, 0, level, index, dst);
        // printf("getarg $%d %d %d\n", dst, level, index);
        break;
      case LOC_GLOBAL:
        set_bc_str(STRING, 0, tn, str);
        set_bc_bireg(GETGLOBAL, 0, dst, tn);
        // printf("string $%d \"%s\"\n", tn, str);
        // printf("getglobal $%d $%d\n", dst, tn);
        break;
      default:
        fprintf(stderr, "compile_bytecode: unexpected case in TOK_NAME\n");
      }
      release_reg(curr_tbl, tn);
    }
    break;

  case TOK_ASSIGN:
    {
      if (root->pn_left->pn_type == TOK_NAME) {
        char *str = atom_to_string(root->pn_left->pn_atom);
        Register t1 = search_unuse();
        Register t2 = search_unuse();
        if (root->pn_op == 0) {
          compile_bytecode(root->pn_right, rho, dst, 0, curr_level);
          compile_assignment(str, rho, dst, curr_level);
        } else {
          compile_bytecode(root->pn_left, rho, t1, 0, curr_level);
          compile_bytecode(root->pn_right, rho, t2, 0, curr_level);
          set_bc_trireg(arith_nemonic_assignment(root->pn_op), 0, dst, t1, t2);
          // printf("%s $%d $%d $%d\n",
          //        arith_nemonic_assignment(root->pn_op), dst, t1, t2);
          compile_assignment(str, rho, dst, curr_level);
        }
        release_reg(curr_tbl, t1);
        release_reg(curr_tbl, t2);
      } else if (root->pn_left->pn_type == TOK_DOT) {
        Register t1 = search_unuse();
        Register t2 = search_unuse();
        Register t3 = search_unuse();
        Register t4 = search_unuse();
        // fprintf(stderr, "q!");
        char *str = atom_to_string(root->pn_left->pn_atom);
        if (root->pn_op == 0) {
          // C[[ e1.x = e2 ]] rho d f
          compile_bytecode(root->pn_left->pn_expr, rho, t1, 0, curr_level);
          compile_bytecode(root->pn_right, rho, dst, 0, curr_level);
          set_bc_str(STRING, 0, t3, str);
          set_bc_trireg(SETPROP, 0, t1, t3, dst);
          // printf("string $%d \"%s\"\n", t3, str);
          // printf("setprop $%d $%d $%d\n", t1, t3, t2);
        } else {
          // C[[ e1.x [arith]= e2 ]] rho d f =
          // C[[ e1 ]] rho t1 False
          compile_bytecode(root->pn_left->pn_expr, rho, t1, 0, curr_level);
          // string t2 name_of(x)
          set_bc_str(STRING, 0, t2, str);
          // getprop t3 t1 t2
          set_bc_trireg(GETPROP, 0, t3, t1, t2);
          // printf("string $%d \"%s\"\n", t2, str);
          // printf("getprop $%d $%d $%d\n", t3, t1, t2);
          // C[[ e2 ]] rho t4 False
          compile_bytecode(root->pn_right, rho, t4, 0, curr_level);
          // [arith] t3 t3 t4
          set_bc_trireg(arith_nemonic_assignment(root->pn_op), 0, dst, t3, t4);
          // setprop t1 t2 t3
          set_bc_trireg(SETPROP, 0, t1, t2, dst);
          // printf("%s $%d $%d $%d\n",
          //        arith_nemonic_assignment( root->pn_op ), t3, t3, t4);
          // printf("setprop $%d $%d $%d\n", t1, t2, t3);
        }
        release_reg(curr_tbl, t1);
        release_reg(curr_tbl, t2);
        release_reg(curr_tbl, t3);
        release_reg(curr_tbl, t4);
      } else if (root->pn_left->pn_type == TOK_LB) {
        // fprintf(stderr, "p!");
        // todo:else if (root->-pn_left->pn_type == TOK_LB){...}
        // C[[ e1[e2] = e3 ]] rho d f
        Register t1 = search_unuse();
        Register t2 = search_unuse();
        Register t3 = search_unuse();
        Register t4 = search_unuse();
        Register t5 = search_unuse();
        Register f_getidx = search_unuse();
        if (root->pn_op == 0) {
          // C[[ e1 ]] rho t1 False
          compile_bytecode(root->pn_left->pn_left, rho, t1, 0, curr_level);
          // C[[ e2 ]] rho t3 False
          compile_bytecode(root->pn_left->pn_right, rho, t3, 0, curr_level);
          // C[[ e3 ]] rho t2 False
          compile_bytecode(root->pn_right, rho, t2, 0, curr_level);
          // gettoidx f_getidx t3
          set_bc_bireg(GETIDX, 0, f_getidx, t3);
          // move $FL t3
          set_bc_bireg(MOVE, 2, 0, t3);
          // send f_getidx 0
          set_bc_regnum(SEND, 0, f_getidx, 0);
          // setfl FL
          set_bc_num(SETFL, 2, 0);
          // move t4 $A
          set_bc_unireg(GETA, 0, t4);
          //set_bc_bireg(MOVE, 0, t4, REG_A);
          // setprop t1 t4 t2
          set_bc_trireg(SETPROP, 0, t1, t4, t2);
        } else {
          // C[[ e1[e2] [arith]= e3 ]] rho d f
          // C[[ e1 ]] rho t1 False
          compile_bytecode(root->pn_left->pn_left, rho, t1, 0, curr_level);
          // C[[ e2 ]] rho t3 False
          compile_bytecode(root->pn_left->pn_right, rho, t3, 0, curr_level);
          // C[[ e3 ]] rho t2 False
          compile_bytecode(root->pn_right, rho, t2, 0, curr_level);
          // gettoidx f_getidx t3
          set_bc_bireg(GETIDX, 0, f_getidx, t3);
          // move $FL t3
          set_bc_bireg(MOVE, 2, 0, t3);
          // send f_getidx 0
          set_bc_regnum(SEND, 0, f_getidx, 0);
          // setfl FL
          set_bc_num(SETFL, 2, 0);
          // move t4 $A  **t4 is property name**
          //set_bc_bireg(MOVE, 0, t4, REG_A);
          set_bc_unireg(GETA, 0, t4);
          // getprop t5 t1 t4
          set_bc_trireg(GETPROP, 0, t5, t1, t4);
          // [arith] t5 t5 t2
          set_bc_trireg(arith_nemonic_assignment(root->pn_op), 0, t5, t5, t4);
          // setprop t1 t4 t5
          set_bc_trireg(SETPROP, 0, t1, t4, t5);
        }
      }
    }
    break;

  case TOK_DOT:
    {
      Register t1 = search_unuse();
      Register t2 = search_unuse();
      char *str = atom_to_string(root->pn_atom);
      compile_bytecode(root->pn_expr, rho, t1, 0, curr_level);
      set_bc_str(STRING, 0, t2, str);
      set_bc_trireg(GETPROP, 0, dst, t1, t2);
      // printf("string $%d \"%s\"\n", t2, str);
      // printf("getprop $%d $%d $%d\n", dst, t1, t2);
      release_reg(curr_tbl, t1);
      release_reg(curr_tbl, t2);
    }
    break;

  case TOK_RC:
    {
      Register objConsStr = search_unuse();
      Register objCons = search_unuse();
      Register propdst = search_unuse();
      Register pstr = search_unuse();
      Register f_getidx = search_unuse();
      JSParseNode *p;

      set_bc_str(STRING, 0, objConsStr, gStringObject);
      set_bc_bireg(GETGLOBAL, 0, objCons, objConsStr);
      set_bc_bireg(NEW, 0, dst, objCons);
      set_bc_bireg(MOVE, 2, 0, dst);
      set_bc_regnum(NEWSEND, 0, objCons, 0);
      //set_bc_num(ADDSP, 0, root->pn_count + 3);
      set_bc_num(SETFL, 2, 0);
      set_bc_unireg(GETA, 0, dst);
      for (p = root->pn_head; p != NULL; p = p->pn_next) {
        switch(p->pn_left->pn_type){
        case TOK_NAME:
        case TOK_STRING:
          {
            char *name = atom_to_string(p->pn_left->pn_atom);
            compile_bytecode(p->pn_right, rho, propdst, 0, curr_level);
            set_bc_str(STRING, 0, pstr, name);
            set_bc_trireg(SETPROP, 0, dst, pstr, propdst);
          }
          break;
        case TOK_NUMBER:
          {
            compile_bytecode(p->pn_right, rho, propdst, 0, curr_level);
            double v = p->pn_left->pn_dval;
            if (v == (double)(long long)v && v < (double)0xffffffff &&
                v > (double) -0x100000000) {
              set_bc_ival(FIXNUM, 0, pstr, (int64_t)v);
            } else {
              set_bc_dval(NUMBER, 0, pstr, v);
            }
            set_bc_bireg(GETIDX, 0, f_getidx, pstr);
            set_bc_bireg(MOVE, 2, 0, pstr);
            set_bc_regnum(SEND, 0, f_getidx, 0);
            set_bc_num(SETFL, 2, 0);
            set_bc_unireg(GETA, 0, pstr);
            set_bc_trireg(SETPROP, 0, dst, pstr, propdst);
          }
          break;
        }
      }
      if (root->pn_count > max_func_fl) {
        max_func_fl = root->pn_count;
      }
    }
    break;

  case TOK_LB:
    {
      Register r1 = search_unuse();
      Register r2 = search_unuse();
      Register f_getidx = search_unuse();
      Register r3 = search_unuse();
      compile_bytecode(root->pn_left, rho, r1, 0, curr_level);
      compile_bytecode(root->pn_right, rho, r2, 0, curr_level);
      // 2011/05/30追記 ここから
      set_bc_bireg(GETIDX, 0, f_getidx, r2);
      set_bc_bireg(MOVE, 2, 0, r2); // flag==2 FL+0 に r2 のデータを移す
      set_bc_regnum(SEND, 0, f_getidx, 0);
      set_bc_num(SETFL, 2, 0);
      //set_bc_bireg(MOVE, 0, r3, REG_A);
      set_bc_unireg(GETA, 0, r3);
      set_bc_trireg(GETPROP, 0, dst, r1, r3);
      //set_bc_trireg(GETPROP, 0, dst, r1, r2);
      if(max_func_fl == 0) max_func_fl+=1;
      // 2011/05/30追記 ここまで
      release_reg(curr_tbl, r1);
      release_reg(curr_tbl, r2);
    }
    break;

  case TOK_RB:
    {
      Register objConsStr = search_unuse();
      Register objCons = search_unuse();
      Register propdst = search_unuse();
      Register dlen = search_unuse();
      JSParseNode *p;
      int count = root->pn_count;
      int i = 0;
      set_bc_ival(FIXNUM, 0, dlen, count);
      set_bc_str(STRING, 0, objConsStr, gStringArray);
      set_bc_bireg(GETGLOBAL, 0, objCons, objConsStr);
      set_bc_bireg(NEW, 0, dst, objCons);
      set_bc_bireg(MOVE, 2, 0, dlen);
      set_bc_bireg(MOVE, 2, -1, dst);
      set_bc_regnum(NEWSEND, 0, objCons, 1);
      //set_bc_num(ADDSP, 0, root->pn_count + 3);
      set_bc_num(SETFL, 2, 0);
      set_bc_unireg(GETA, 0, dst);
      for (p = root->pn_head; p != NULL; p = p->pn_next, i++) {
        if (p->pn_type == TOK_COMMA) {
          set_bc_cons(SPECCONST, 0, propdst, UNDEFINED);
        } else {
          compile_bytecode(p, rho, propdst, 0, curr_level);
        }
        set_bc_trireg(SETARRAY, 0, dst, i, propdst);
      }
      if (1 > max_func_fl) {
        max_func_fl = 1;
      }
    }
    break;

  case TOK_INC:
  case TOK_DEC:
    {
      switch (root->pn_op) {
      case JSOP_INCNAME:
      case JSOP_DECNAME:
        {
          Register t1 = search_unuse();
          Register tone = search_unuse();
          char *str = atom_to_string(root->pn_kid->pn_atom);	
          double one = 1.0;
          compile_bytecode(root->pn_kid, rho, t1, 0, curr_level);
          set_bc_ival(FIXNUM, 0, tone, (int64_t)one);
          set_bc_trireg(arith_nemonic_prefix(root->pn_type), 0, dst, t1, tone);
          // printf("number $%d %lf\n", tone, one);
          // printf("%s $%d $%d $%d\n", arith_nemonic_prefix(root->pn_type), dst, t1, tone);
          compile_assignment(str, rho, dst, curr_level);
          release_reg(curr_tbl, t1);
          release_reg(curr_tbl, tone);
          release_reg(curr_tbl, tn);
        }
        break;
      case JSOP_INCPROP:
      case JSOP_DECPROP:
        {
          Register t1 = search_unuse();
          Register t2 = search_unuse();
          Register tone = search_unuse();
          Register tn = search_unuse();
          char *str = atom_to_string(root->pn_kid->pn_atom);	
          double one = 1.0;
          compile_bytecode(root->pn_kid->pn_expr, rho, t2, 0, curr_level);
          set_bc_str(STRING, 0, tn, str);
          set_bc_trireg(GETPROP, 0, t1, t2, tn);
          set_bc_ival(FIXNUM, 0, tone, (int64_t)one);
          //set_bc_dval(FIXNUM, 0, tone, one);
          set_bc_trireg(arith_nemonic_prefix(root->pn_type), 0, dst, t1, tone);
          set_bc_trireg(SETPROP, 0, t2, tn, dst);
          /*
            printf("string $%d \"%s\"\n", tn, str);
            printf("getprop $%d $%d $%d\n", t1, t2, tn);
            printf("number $%d %lf\n", tone, one);
            printf("%s $%d $%d $%d\n", 
                   arith_nemonic_prefix(root->pn_type), dst, t1, tone);
            printf("setprop $%d $%d $%d\n", t2, tn, dst);	    
          */
          release_reg(curr_tbl, t1);
          release_reg(curr_tbl, t2);
          release_reg(curr_tbl, t3);
          release_reg(curr_tbl, tone);
          release_reg(curr_tbl, tn);
        }
        break;
      case JSOP_NAMEINC:
      case JSOP_NAMEDEC:
        {
          Register t1 = search_unuse();
          Register tone = search_unuse();
          char *str = atom_to_string(root->pn_kid->pn_atom);	
          double one = 1.0;
          compile_bytecode(root->pn_kid, rho, dst, 0, curr_level);
          set_bc_ival(FIXNUM, 0, tone, (int64_t)one);
          // set_bc_dval(FIXNUM, 0, tone, one);
          set_bc_trireg(arith_nemonic_prefix(root->pn_type), 0, t1, dst, tone);
          // printf("number $%d %lf\n", tone, one);
          // printf("%s $%d $%d $%d\n",
          //        arith_nemonic_prefix(root->pn_type), t1, dst, tone);
          compile_assignment(str, rho, t1, curr_level);
          release_reg(curr_tbl, t1);
          release_reg(curr_tbl, tone);
          release_reg(curr_tbl, tn);
        }
        break;
      case JSOP_PROPINC:
      case JSOP_PROPDEC:
        {
          Register t1 = search_unuse();
          Register t2 = search_unuse();
          Register tone = search_unuse();
          Register tn = search_unuse();
          char *str = atom_to_string(root->pn_kid->pn_atom);	
          double one = 1.0;
          compile_bytecode(root->pn_kid->pn_expr, rho, t2, 0, curr_level);
          set_bc_str(STRING, 0, tn, str);
          set_bc_trireg(GETPROP, 0, dst, t2, tn);
          set_bc_ival(FIXNUM, 0, tone, (int64_t)one);
          //set_bc_dval(FIXNUM, 0, tone, one);
          set_bc_trireg(arith_nemonic_prefix(root->pn_type), 0, t1, dst, tone);
          set_bc_trireg(SETPROP, 0, t2, tn, t1);
          /*
            printf("string $%d \"%s\"\n", tn, str);
            printf("getprop $%d $%d $%d\n", dst, t2, tn);
            printf("number $%d %lf\n", tone, one);
            printf("%s $%d $%d $%d\n",
                   arith_nemonic_prefix(root->pn_type), t1, dst, tone);
            printf("setprop $%d $%d $%d\n", t2, tn, t1);	    
          */
          release_reg(curr_tbl, t1);
          release_reg(curr_tbl, t2);
          release_reg(curr_tbl, t3);
          release_reg(curr_tbl, tone);
          release_reg(curr_tbl, tn);
        }
        break;
      default:
        break;
      }
    }
    break;
    
  case TOK_UNARYOP:
    switch (root->pn_op) {
    case JSOP_NOT:
      {
        Register t1 = search_unuse();
        compile_bytecode(root->pn_kid, rho, t1, 0, curr_level);
        set_bc_bireg(NOT, 0, dst, t1);
      }
      break;
    case JSOP_NEG:
      {
        Register t1 = search_unuse();
        Register tone = search_unuse();
        double mone = -1.0;
        compile_bytecode(root->pn_kid, rho, t1, 0, curr_level);
        set_bc_ival(FIXNUM, 0, tone, (int64_t)mone);
        //set_bc_dval(FIXNUM, 0, tone, mone);
        set_bc_trireg(MUL, 0, dst, t1, tone);
        //printf("number $%d %lf\n", tone, mone);
        //printf("mul $%d $%d $%d\n", dst, t1, tone);
        release_reg(curr_tbl, t1);
        release_reg(curr_tbl, tone);
      }
      break;//2011/07/13 追記 break忘れ;
    case JSOP_TYPEOF:
      {
        compile_bytecode(root->pn_kid, rho, dst, 0, curr_level);
        set_bc_bireg(TYPEOF, 0, dst, dst);
        //printf("typeof $%d $%d\n", dst, dst);
      }
      break;
    case JSOP_VOID:
      {
        compile_bytecode(root->pn_kid, rho, dst, 0, curr_level);
        set_bc_cons(SPECCONST, 0, dst, UNDEFINED);
        //printf("const $%d undefined\n", dst);
      }
      break;
    default:
      compile_bytecode(root->pn_kid, rho, dst, 0, curr_level);
    }
    break;

  case TOK_AND:
    {
      int l1;
      int j1;
      int label = curr_label++;
      compile_bytecode(root->pn_left, rho, dst, 0, curr_level);
      j1 = curr_code_num;
      set_bc_regnum(JUMPFALSE, 1, dst, label);
      //printf("jumpfalse $%d L\n", dst);
      compile_bytecode(root->pn_right, rho, dst, tailflag, curr_level);
      l1 = curr_code_num;
      dispatch_label(j1, l1);
      //set_label(label);
#if DEBUG4
      printf("%d %d\n", curr_code, curr_code_num);
      printf("%d\n", bytecode[curr_code][curr_code_num].label);
#endif
      //printf("L:\n");
    }
    break;

  case TOK_OR:
    {
      int l1;
      int j1;
      int label = curr_label++;
      compile_bytecode(root->pn_left, rho, dst, 0, curr_level);
      j1 = curr_code_num;
      set_bc_regnum(JUMPTRUE, 1, dst, label);
      //printf("jumptrue $%d L\n", dst);
      compile_bytecode(root->pn_right, rho, dst, tailflag, curr_level);
      l1 = curr_code_num;
      dispatch_label(j1, l1);
      //set_label(label);
      //printf("L:\n");
    }
    break;

  case TOK_RELOP:
    {
      Register t1 = search_unuse();
      Register t2 = search_unuse();
      switch (root->pn_op) {
      case JSOP_LT:
      case JSOP_LE:
        compile_bytecode(root->pn_left, rho, t1, 0, curr_level);
        compile_bytecode(root->pn_right, rho, t2, 0, curr_level);
        set_bc_trireg(root->pn_op == JSOP_LT ? LESSTHAN : LESSTHANEQUAL, 0,
                      dst, t1, t2);
        // printf("%s $%d $%d $%d\n",
        //        root->pn_op == JSOP_LT ? "lessthan" : "lessthanequal",
        //        dst, t1, t2);
        break;
      case JSOP_GT:
      case JSOP_GE:
        compile_bytecode(root->pn_left, rho, t1, 0, curr_level);
        compile_bytecode(root->pn_right, rho, t2, 0, curr_level);
        set_bc_trireg(root->pn_op == JSOP_GT ? LESSTHAN : LESSTHANEQUAL, 0,
                      dst, t2, t1);
        // printf("%s $%d $%d $%d\n",
        //        root->pn_op == JSOP_GT ? "lessthan" : "lessthanequal",
        //        dst, t2, t1);
        break;
      }
      release_reg(curr_tbl, t1);
      release_reg(curr_tbl, t2);
    }
    break;

  case TOK_EQOP:
    {
      Register t1 = search_unuse();
      Register t2 = search_unuse();
      compile_bytecode(root->pn_left, rho, t1, 0, curr_level);
      compile_bytecode(root->pn_right, rho, t2, 0, curr_level);
      if ( root->pn_op == JSOP_EQ ) {
        int ffin1, ffin2, fconvb, fsecond;
        int tfin1, tfin2, tconvb, tsecond;
        int lfin1 = curr_label++;
        int lfin2 = curr_label++;
        int lconvb = curr_label++;
        int lsecond = curr_label++;
        Register ts = search_unuse();
        Register vs = search_unuse();
        Register to = search_unuse();
        Register fc = search_unuse();
        Register ca = search_unuse();
        Register cb = search_unuse();
        set_bc_trireg(EQUAL, 0, dst, t1, t2);    
        set_bc_bireg(ISUNDEF, 0, ts, dst);
        ffin1 = curr_code_num;
        set_bc_regnum(JUMPFALSE, 1, ts, lfin1);
        set_bc_str(STRING, 0, vs, "valueOf");
        set_bc_bireg(ISOBJECT, 0, to, t1);
        fconvb = curr_code_num;
        set_bc_bireg(JUMPFALSE, 1, to, lconvb);
        set_bc_trireg(GETPROP, 0, fc, t1, vs);
        set_bc_bireg(MOVE, 2, 0, t1);
        set_bc_regnum(SEND, 0, fc, 0);
        //set_bc_bireg(MOVE, 0, ca, REG_A);
        set_bc_unireg(GETA, 0, ca);
        set_bc_bireg(MOVE, 0, cb, t2);
        fsecond = curr_code_num;
        set_bc_num(JUMP, 1, lsecond);
        //convb
        tconvb = curr_code_num;
        set_bc_trireg(GETPROP, 0, fc, t2, vs);
        set_bc_bireg(MOVE, 2, 0, t2);
        set_bc_regnum(SEND, 0, fc, 0);
        //set_bc_bireg(MOVE, 0, cb, REG_A);
        set_bc_unireg(GETA, 0, cb);
        set_bc_bireg(MOVE, 0, ca, t1);
        //tsecond
        tsecond = curr_code_num;
        set_bc_trireg(EQUAL, 0, dst, ca, cb);    
        set_bc_bireg(ISUNDEF, 0, ts, dst);
        ffin2 = curr_code_num;
        set_bc_regnum(JUMPFALSE, 1, ts, lfin2);
        set_bc_str(ERROR, 0, dst, "EQUAL_GETTOPRIMITIVE");
        // tfin
        tfin1 = tfin2 = curr_code_num;
        dispatch_label(ffin1, tfin1);
        dispatch_label(ffin2, tfin2);
        dispatch_label(fconvb, tconvb);
        dispatch_label(fsecond, tsecond);
      } else {
        set_bc_trireg(EQ, 0, dst, t1, t2);    
      }
      // printf("%s $%d $%d $%d\n", root->pn_op == JSOP_EQ ? "equal" : "eq",
      //        dst, t1, t2);
		}
    break;

  case TOK_HOOK:
    {
      Register t = search_unuse();
      int label1 = curr_label++;
      int label2 = curr_label++;
      int j1, j2,l1,l2;
      compile_bytecode(root->pn_kid1, rho, t, 0, curr_level);
      j1 = curr_code_num;
      set_bc_regnum(JUMPFALSE, 1, t, label1);
      //printf("jumpfalse $%d L1\n",t);/*label*/
      l1 = curr_code_num;
      //set_label(l1);
      //printf("L1:\n");
      compile_bytecode(root->pn_kid2, rho, dst, tailflag, curr_level);
      j2 = curr_code_num;
      set_bc_num(JUMP, 1, label2);
      //printf("jump L2\n");
      compile_bytecode(root->pn_kid3, rho, dst, tailflag, curr_level);
      l2 = curr_code_num;
      //set_label(l2);
      //printf("L2:\n");
      release_reg(curr_tbl, t);
      dispatch_label(j1, l1);
      dispatch_label(j2, l2);
    }
    break;

  case TOK_IF:
    {
      int elseflag = (int)(intptr_t)(root->pn_kid3);
      Register t = search_unuse();
      int l1 = curr_label++;
      int l2 = curr_label++;
      int j1, j2;
      compile_bytecode(root->pn_kid1, rho, t, 0, curr_level);
      j1 = curr_code_num;
      set_bc_regnum(JUMPFALSE, 1, t, l1);
      //printf("jumpfalse $%d L1\n",t);
      compile_bytecode(root->pn_kid2, rho, dst, 0, curr_level);
      if (elseflag) {
        j2 = curr_code_num;
        set_bc_num(JUMP, 1, l2);
        //printf("jump L2\n");
      }
      l1 = curr_code_num;
      //set_label(l1);
      //printf("L1:\n");
      if (elseflag) {
        compile_bytecode(root->pn_kid3, rho, dst, 0, curr_level);
        l2 = curr_code_num;
        //set_label(l2);
        //printf("L2:\n");
      }
      dispatch_label(j1, l1);
      if (elseflag) {
        dispatch_label(j2, l2);
      }	
      release_reg(curr_tbl, t);
    }
    break;

  case TOK_FOR:
    if (root->pn_left->pn_type == TOK_RESERVED) {
      Register t = search_unuse();
      int l1 = curr_label++;
      int l2 = curr_label++;
      int j1, j2;
      compile_bytecode(root->pn_left->pn_kid1, rho, t, 0, curr_level);
      j1 = curr_code_num;
      set_bc_num(JUMP, 1, l1);
      //printf("jump L1\n");
      l2 = curr_code_num;
      //set_label(l2);
      //printf("L2:\n");
      compile_bytecode(root->pn_right, rho, dst, 0, curr_level);
      compile_bytecode(root->pn_left->pn_kid3, rho, t, 0, curr_level);
      l1 = curr_code_num;
      //set_label(l1);
      //printf("L1:\n");
      compile_bytecode(root->pn_left->pn_kid2, rho, t, 0, curr_level);
      j2 = curr_code_num;
      set_bc_regnum(JUMPTRUE, 1, t, l2);
      //printf("jumptrue $%d L2\n",t);
      release_reg(curr_tbl, t);
      dispatch_label(j1, l1);
      dispatch_label(j2, l2);
    } else {
      if (func_tbl[curr_code].existClosure) {
        fprintf(stderr, "hogehoge");
      } else {
        fprintf(stderr, "fugafuga");
        Register obj, iter, name, namep;
        int l1 = curr_label++;
        int l2 = curr_label++;
        int f1, f2;
        obj = search_unuse();
        iter = search_unuse();
        name = search_unuse();
        namep = search_unuse();
        compile_bytecode(root->pn_left->pn_right, rho, obj, 0, curr_level);
        char *str = atom_to_string(root->pn_left->pn_left->pn_atom);
        rho = env_expand(str, LOC_REGISTER, curr_level, 1, name, rho);
        set_bc_bireg(MAKEITERATOR, 0, obj, iter);
        l1 = curr_code_num;
        set_bc_trireg(NEXTPROPNAME, 0, obj, iter, name);
        set_bc_bireg(ISUNDEF, 0, namep, name);
        f2 = curr_code_num;
        set_bc_regnum(JUMPTRUE, 1, namep, l2);
        compile_bytecode(root->pn_right, rho, dst, 0, curr_level);
        f1 = curr_code_num;
        set_bc_num(JUMP, 1, l1);
        l2 = curr_code_num;
        dispatch_label(f1, l1);
        dispatch_label(f2, l2);
      }
    }
    break;

  case TOK_WHILE:
    {
      Register t = search_unuse();
      int l1 = curr_label++;
      int l2 = curr_label++;
      int j1, j2;
      j1 = curr_code_num;
      set_bc_num(JUMP, 1, l1);
      //printf("jump L1\n");
      l2 = curr_code_num;
      //set_label(l2);
      //printf("L2:\n");
      compile_bytecode(root->pn_right, rho, dst, 0, curr_level);
      l1 = curr_code_num;
      //set_label(l1);
      //printf("L1:\n");
      compile_bytecode(root->pn_left, rho, t, 0, curr_level);
      j2 = curr_code_num;
      set_bc_regnum(JUMPTRUE, 1, t, l2);
      //printf("jumptrue $%d L2\n", t);
      release_reg(curr_tbl, t);
      dispatch_label(j1, l1);
      dispatch_label(j2, l2);
    }
    break;

  case TOK_DO:
    {
      Register t = search_unuse();
      int l1 = curr_label++;
      int j1;
      l1 = curr_code_num;
      //set_label(l1);
      //printf("L1:\n");
      compile_bytecode(root->pn_left, rho, dst, 0, curr_level);
      compile_bytecode(root->pn_right, rho, t, 0, curr_level);
      j1 = curr_code_num;
      set_bc_regnum(JUMPTRUE, 1, t, l1);
      //printf("jumptrue $%d L1\n", t);
      release_reg(curr_tbl, t);	
      dispatch_label(j1, l1);
    }
    break;

  case TOK_LP:
    {
      if (root->pn_head->pn_type == TOK_DOT) {
        int count = root->pn_count;
        Register tm = search_unuse();
        Register ts = search_unuse();
        Register *tmp = malloc(sizeof(Register) * (count));
        int i;
        JSParseNode *p;
        JSParseNode *methodnode = root->pn_head;
        char *m_name = atom_to_string(methodnode->pn_atom);
        for (i = 0; i < root->pn_count; i++)
          tmp[i] = search_unuse();
        compile_bytecode(methodnode->pn_expr, rho, tmp[0], 0, curr_level);
        set_bc_str(STRING, 0, ts, m_name);
        set_bc_trireg(GETPROP, 0, tm, tmp[0], ts);
        for (p = root->pn_head->pn_next, i = 1; p != NULL;
             p = p->pn_next, i++) { 
          compile_bytecode(p, rho, tmp[i], 0, curr_level);
        }
        if (tailflag) {
          set_bc_bireg(MOVE, 0, 1, tmp[0]);
          for (i = 0; i < root->pn_count - 1; i++) {
            set_bc_bireg(MOVE, 0, i+2, tmp[i+1]);
            //printf("move $%d $%d\n", i+2, tmp[i+1]);
          }
          set_bc_regnum(TAILSEND, 0, tm, root->pn_count - 1);
          //printf("tailcall $%d %d\n", tmp[0], root->pn_count - 1);
        } else {
          set_bc_bireg(MOVE, 2, -root->pn_count + 1, tmp[0]);
          for (i = 0; i < root->pn_count - 1; i++) {
            set_bc_bireg(MOVE, 2, -root->pn_count + 2 + i, tmp[i+1]);
            //printf("move $%d $%d\n",
            //       fl_tbl[curr_tbl] - root->pn_count +2 + i, tmp[i+1]);
          }
          set_bc_regnum(SEND, 0, tm, root->pn_count - 1);
          //set_bc_num(ADDSP, 0, root->pn_count + 3);
          set_bc_num(SETFL, 2, 0);
          //set_bc_bireg(MOVE, 0, dst, REG_A);
          set_bc_unireg(GETA, 0, dst);
          /*
            printf("call $%d %d\n", tmp[0], root->pn_count - 1);
            printf("addsp %d\n", root->pn_count + 3);
            printf("move $%d $A\n", dst, REG_A);
          */
          if (root->pn_count > max_func_fl) {
            max_func_fl = root->pn_count;
          }
        }
        for (i = 0; i < root->pn_count; i++) {
          release_reg(curr_tbl, i);
        }
        free(tmp);	  
      } else {
        int count = root->pn_count;
        Register *tmp = malloc(sizeof(Register) * (count));
        int i;
        JSParseNode *p;
        for (i = 0; i < root->pn_count; i++)
          tmp[i] = search_unuse();
        for (p = root->pn_head, i=0; p != NULL; p = p->pn_next, i++) { 
          compile_bytecode(p, rho, tmp[i], 0, curr_level);
        }
        if (tailflag) {
          for (i = 0; i < root->pn_count - 1; i++) {
            set_bc_bireg(MOVE, 0, i+2, tmp[i+1]);
            //printf("move $%d $%d\n", i+2, tmp[i+1]);
          }
          set_bc_regnum(TAILCALL, 0, tmp[0], root->pn_count - 1);
          //printf("tailcall $%d %d\n", tmp[0], root->pn_count - 1);
        } else {
          for (i = 0; i < root->pn_count - 1; i++) {
            set_bc_bireg(MOVE, 2, -root->pn_count + 2 + i, tmp[i+1]);
            // printf("move $%d $%d\n",
            //         fl_tbl[curr_tbl] - root->pn_count +2 + i, tmp[i+1]);
          }
          set_bc_regnum(CALL, 0, tmp[0], root->pn_count - 1);
          //set_bc_num(ADDSP, 0, root->pn_count + 3);
          set_bc_num(SETFL, 2, 0);
          //set_bc_bireg(MOVE, 0, dst, REG_A);
          set_bc_unireg(GETA, 0, dst);
          /*
            printf("call $%d %d\n", tmp[0], root->pn_count - 1);
            printf("addsp %d\n", root->pn_count + 3);
            printf("move $%d $A\n", dst, REG_A);
          */
          if (root->pn_count > max_func_fl) {
            max_func_fl = root->pn_count;
          }
        }
        for (i = 0; i < root->pn_count; i++) {
          release_reg(curr_tbl, i);
        }
        free(tmp);
      }
    }
    break;

  case TOK_TRY:
    {
      int j1, j2;
      int l1 = curr_label++;
      int l2 = curr_label++;
      int level, offset, index;
      char* str;
      Environment rho2;
      Register err = search_unuse();
      Location loc;
      j1 = curr_code_num;
      set_bc_num(TRY, 1, l1);
      compile_bytecode(root->pn_kid1, rho, dst, 0, curr_level);
      set_bc_unireg(FINALLY, 0, 0);
      j2 = curr_code_num;
      set_bc_num(JUMP, 1, l2);
      str = atom_to_string(root->pn_kid2->pn_kid1->pn_atom);
      loc = &func_tbl[curr_code] == func_tbl ? LOC_LOCAL :
            func_tbl[curr_code].existClosure ? LOC_LOCAL : LOC_REGISTER;
      l1 = curr_code_num;
      set_bc_num(SETFL, 2, 0);
      set_bc_unireg(GETERR, 0, err);
      if (loc == LOC_LOCAL) {
        rho2 = env_expand(str, loc, curr_level, ++var_num[curr_code], 1, rho);
        env_lookup(rho2, curr_level, str, &level, &offset, &index);
        set_bc_var(SETLOCAL, 0, level, offset, err);
      } else {
        rho2 = env_expand(str, loc, curr_level, 1, err, rho);
      }
      compile_bytecode(root->pn_kid2->pn_kid3, rho2, dst, 0, curr_level);
      l2 = curr_code_num;
      compile_bytecode(root->pn_kid3, rho, dst, 0, curr_level);
      dispatch_label(j1, l1);
      dispatch_label(j2, l2);
    }
    break;

  case TOK_THROW:
    {
      Register th = search_unuse();
      compile_bytecode(root->pn_kid, rho, th, 0, curr_level);
      set_bc_unireg(THROW, 0, th);
    }
    break;

  case TOK_FUNCTION:
    {
      set_bc_regnum(MAKECLOSURE, 0, dst, 
                    add_function_tbl(root, rho, curr_level+1));
      // printf("makeclosure $%d %d\n", dst,
      //        add_function_tbl(root, rho, curr_level+1));
    }
    break;

  case TOK_NEW:
    {
      Register *tmp = malloc(sizeof(Register) * (root->pn_count));
      Register ts = search_unuse();
      Register tg = search_unuse();
      Register tins = search_unuse();
      Register retr = search_unuse();
      int i;
      JSParseNode *p;
      int l1 = curr_label++;
      int j1;
      for (i = 0; i < root->pn_count; i++)
        tmp[i] = search_unuse();
      for (p = root->pn_head, i=0; p != NULL; p = p->pn_next, i++) { 
        compile_bytecode(p, rho, tmp[i], 0, curr_level);  
      }
      for (i = 0; i < root->pn_count - 1; i++) {
        set_bc_bireg(MOVE, 2, -root->pn_count + 2 + i, tmp[i + 1]);
        //printf("move $%d $%d\n", fl_tbl[curr_tbl] - root->pn_count + 2 + i,
        //       tmp[i + 1]);
      }
      set_bc_bireg(NEW, 0, dst, tmp[0]);
      set_bc_bireg(MOVE, 2, -(root->pn_count - 1), dst);
      set_bc_regnum(NEWSEND, 0, tmp[0], root->pn_count - 1);
      //set_bc_num(ADDSP, 0, root->pn_count + 3);
      set_bc_num(SETFL, 2, 0);
      set_bc_str(STRING, 0, ts, "Object");
      set_bc_bireg(GETGLOBAL, 0, tg, ts);
      set_bc_unireg(GETA, 0, retr);
      set_bc_trireg(INSTANCEOF, 0, tins, retr, tg);
      j1 = curr_code_num;
      set_bc_regnum(JUMPFALSE, 1, tins, l1);
      //set_bc_bireg(MOVE, 0, dst, REG_A);
      set_bc_unireg(GETA, 0, dst);
      l1 = curr_code_num;
      if (root->pn_count > max_func_fl) {
        max_func_fl = root->pn_count;
      }
      //set_label(l1);
      /*
        printf("new $%d $%d\n", dst, tmp[0]);
        printf("move $%d $%d\n", fl_tbl[curr_tbl] - (root->pn_count - 1), dst);
        printf("send $%d %d\n", tmp[0], root->pn_count - 1);
        printf("addsp %d\n", root->pn_count + 3);
        printf("string $%d \"%s\"\n", ts, "Object");
        printf("instanceof $%d $A $%d\n", tins, ts);
        printf("jumpfalse $%d L\n", tins);
        printf("move $%d $A\n", dst);
        printf("L:\n");
      */
      dispatch_label(j1, l1);
    }
    break;

  case TOK_RETURN:
    {
      Register t;
      t = search_unuse();
      compile_bytecode(root->pn_kid, rho, t, 1, curr_level);
      //set_bc_bireg(MOVE, 0, REG_A, t);
      set_bc_unireg(SETA, 0, t);
      set_bc_unireg(RET, 0, -1);
      //printf("ret\n");
    }
    break;

  case TOK_VAR:
    {
      JSParseNode *p;
      for (p = root->pn_head; p != NULL; p = p->pn_next) {
        if (p->pn_expr != NULL) {
          if (root->pn_left->pn_type == TOK_NAME) {
            char *str = atom_to_string(root->pn_left->pn_atom);
            compile_bytecode(p->pn_expr, rho, dst, 0, curr_level);
            compile_assignment(str, rho, dst, curr_level);
            /*
              if (p->pn_op == 0) {
              compile_bytecode(p->pn_expr, rho, dst, 0, curr_level);
              compile_assignment(str, rho, dst, curr_level);
              } else {
              compile_bytecode(p, rho, t1, 0, curr_level);
              compile_bytecode(p->pn_expr, rho, t2, 0, curr_level);
              printf("%s $%d $%d $%d\n", arith_nemonic_assignment(p->pn_op), dst, t1, t2);
              compile_assignment(str, rho, dst, curr_level);
              }
            */
            release_reg(curr_tbl, t1);
            release_reg(curr_tbl, t2);
          }
        }
      }
    }
    break;

  default:
    switch (root->pn_arity) {
    case PN_UNARY:
      compile_bytecode(root->pn_kid, rho, dst, tailflag, curr_level);
      break;
    case PN_BINARY:
      compile_bytecode(root->pn_left, rho, dst, tailflag, curr_level);
      compile_bytecode(root->pn_right, rho, dst, tailflag, curr_level);
      break;
    case PN_TERNARY:
      compile_bytecode(root->pn_kid1, rho, dst, tailflag, curr_level);
      compile_bytecode(root->pn_kid2, rho, dst, tailflag, curr_level);
      compile_bytecode(root->pn_kid3, rho, dst, tailflag, curr_level);
      break;
    case PN_LIST:
      {
        JSParseNode * p;
        for (p = root->pn_head; p != NULL; p = p->pn_next) {
          compile_bytecode(p, rho, dst, tailflag, curr_level);
        }
      }	
      break;
    case PN_FUNC:
    case PN_NAME:
      if (root->pn_expr != NULL) {
        compile_bytecode(root->pn_expr, rho, dst, tailflag, curr_level);
      }
      break;
    case PN_NULLARY:
      break;
    }
    break;
  }
}

void compile_function(Function_tbl func_tbl, int index)
{
  Environment rho = func_tbl.rho;
  curr_code = index;
  curr_code_num = 0;
  max_func_fl = 0;
  Register retu;
  bool existClosure;
  bool useArguments;
  //  printf("call_entry_%d:\n", index);
  existClosure = func_tbl.existClosure ||
                 searchFunctionDefinition(func_tbl.node->pn_body);
  useArguments = searchUseArguments(func_tbl.node->pn_body);
#ifndef OPT_FFRAME
  existClosure = true;
#endif
  set_bc_unireg(GETGLOBALOBJ, 0, 1);
  // set_bc_cons(SPECCONST, 0, 1, Null);
  if (existClosure || useArguments) {
    set_bc_unireg(NEWARGS, 0, 0);
    func_tbl.existClosure = true;
  }
  //set_bc_bireg(NEWARGS, 0, REG_A, REG_A);
  set_bc_num(SETFL, 2, 0);
  //set_bc_var(SETLOCAL, 0, 0, 1, REG_A);
  /*
    printf("const $%d %s\n", 1, "null");
    printf("send_entry_%d:\n", index);
    printf("newargs $%d $%d\n", REG_A, REG_A);
    printf("setfl FL\n");
    printf("setlocal %d %d $%d\n", 0, 1, REG_A);
  */
  init_reg_tbl(func_tbl.level);
  // fprintf(stderr, "compile_function:function_%d:func_tbl.existClosure?%d\n",
  //         index, (int)func_tbl.existClosure);
  // fprintf(stderr, "compile_function:function_%d:existClosure?%d\n",
  //         index, (int)existClosure);
  // fprintf(stderr, "compile_function:function_%d:useArguments?%d\n",
  //         index, (int)useArguments);
  /*引数をrhoに登録*/
  {
    JSObject * object = ATOM_TO_OBJECT(func_tbl.node->pn_funAtom);
    JSFunction * function = (JSFunction *) JS_GetPrivate(context, object);
    JSAtom ** params = malloc(function->nargs * sizeof(JSAtom *));
    int i;
    for (i = 0; i < function->nargs; i++) {
      params[i] = NULL;
    }
    JSScope * scope = OBJ_SCOPE(object);
    JSScopeProperty * scope_property;
    for (scope_property = SCOPE_LAST_PROP(scope); scope_property != NULL;
         scope_property = scope_property->parent) {
      if (scope_property->getter != js_GetArgument) {
        continue;
      }
      params[(uint16) scope_property->shortid] =
        JSID_TO_ATOM(scope_property->id);
    }
    for (i = 0; i < function->nargs; i++) {
      char *name = atom_to_string(params[i]);
      if (existClosure || useArguments) {
        rho = env_expand(name, LOC_ARG, func_tbl.level, 1, i, rho);
      } else {
        Register r = search_unuse();
        rho = env_expand(name, LOC_REGISTER, func_tbl.level, 1, r, rho);
        // fprintf(stderr, "compile_function:a%d:%s -> r_%d\n", i, name, r);
      }
    }
    free(params);
  }
  /*変数をrhoに登録*/
  {
    //var_to_rho(func_tbl.node->pn_body, rho, );
    JSParseNode *p;
    int i;
    for (p = func_tbl.node->pn_body->pn_head, i = 2; p != NULL;
         p = p->pn_next) {
      if (p->pn_type == TOK_VAR) {
        JSParseNode *q;
        for (q = p->pn_head; q != NULL; q = q->pn_next) {
          char *name = atom_to_string(q->pn_atom);
          if (existClosure || useArguments) {
            rho = env_expand(name, LOC_LOCAL, func_tbl.level, i++, 1, rho);
          } else {
            Register r = search_unuse();
            rho = env_expand(name, LOC_REGISTER, func_tbl.level, 1, r, rho);
            // fprintf(stderr, "compile_function:v%d:%s -> r_%d\n", i, name, r);
          }
        }
      }
    }
    // i を保存
    var_num[index] = i;
  }
  compile_bytecode(func_tbl.node->pn_body, rho, search_unuse(), 0,
                   func_tbl.level);
  retu = search_unuse();
  set_bc_cons(SPECCONST, 1, retu, UNDEFINED);
  set_bc_unireg(SETA, 0, retu);
  set_bc_unireg(RET, 0, 0);
  /*
    printf("const $%d %s\n", REG_A, "undefined");
    printf("ret\n");
  */
}

#define ENCODESTRLEN  1024

static char encoded_string[ENCODESTRLEN];

#define ddd(x)  (*d++ = '\\', *d++ = (x))

char *encode_str(char* src)
{
  int c;
  char *d;

  d = encoded_string;
  while ((c = *src++) != '\0') {
    //  printf("c = %d = 0x%x\n", c, c);
    switch (c) {
    case '\0': ddd('0'); break;
    case '\a': ddd('a'); break;
    case '\b': ddd('b'); break;
    case '\f': ddd('f'); break;
    case '\n': ddd('n'); break;
    case '\r': ddd('r'); break;
    case '\t': ddd('t'); break;
    case '\v': ddd('v'); break;
    case '\\': ddd('\\'); break;
    case '\'': ddd('\''); break;
    case '\"': ddd('\"'); break;
    default:
      if ((' ' <= c) && (c <= '~')) *d++ = c;
      else {
        *d++ = '\\';
        *d++ = 'x';
        *d++ = "0123456789abcdef"[(c >> 4) & 0xf];
        *d++ = "0123456789abcdef"[c & 0xf];
      }
      break;
    }
  }
  *d = '\0';
  return encoded_string;
}

// 2013/08/20 Iwasaki
//   rewrote not to produce S-expr format
void print_bytecode(Bytecode bytecode[201][MAX_BYTECODE], int num, FILE *f)
{
  int i, j;

  if (output_format == OUTPUT_SEXPR) fprintf(f, "(\n");
  else fprintf(f, "funcLength %d\n", num);
  for(i = 0; i < num; i++) {
#if DEBUG4
    fprintf(f, "code_num[%d] = %d\n", i, code_num[i]);
#endif
    if (output_format == OUTPUT_SEXPR) {
      fprintf(f, "(");
      if(i == 0) fprintf(f, "0 0 %d\n(", var_num[i]);
      else fprintf(f, "0 1 %d\n(",var_num[i]);
    } else {
      fprintf(f, "callentry 0\n");
      fprintf(f, "sendentry %d\n", i == 0? 0: 1);
      fprintf(f, "numberOfLocals %d\n", var_num[i]);
      fprintf(f, "numberOfInstruction %d\n", code_num[i]);
    }

    for (j = 0; j < code_num[i]; j++) {
      if (bytecode[i][j].label != 0) {
	fprintf(f, "%d:\n", bytecode[i][j].label);
      }

      if (output_format == OUTPUT_SEXPR) fprintf(f, "(");

      switch (bytecode[i][j].nemonic) {
      case ADD:
      case SUB:
      case MUL:
      case DIV:
      case MOD:
      case BITAND:
      case BITOR:
      case LEFTSHIFT:
      case RIGHTSHIFT:
      case UNSIGNEDRIGHTSHIFT:
	fprintf(f, "%s %d %d %d", nemonic_to_str(bytecode[i][j].nemonic), 
	       bytecode[i][j].bc_u.trireg.r1, bytecode[i][j].bc_u.trireg.r2, 
	       bytecode[i][j].bc_u.trireg.r3);
	break;
      case FIXNUM:
	// fprintf(f, "%s %d %lld", nemonic_to_str(bytecode[i][j].nemonic), 
	fprintf(f, "%s %d %"PRId64, nemonic_to_str(bytecode[i][j].nemonic),
	       bytecode[i][j].bc_u.ival.dst, bytecode[i][j].bc_u.ival.val);
	break;
      case NUMBER:
	fprintf(f, "%s %d %g", nemonic_to_str(bytecode[i][j].nemonic), 
	       bytecode[i][j].bc_u.dval.dst, bytecode[i][j].bc_u.dval.val);
	break;
      case STRING:
      case ERROR:
        if (output_format == OUTPUT_SEXPR)
          fprintf(f, "%s %d \"%s\"",  nemonic_to_str(bytecode[i][j].nemonic),
                 bytecode[i][j].bc_u.str.dst, bytecode[i][j].bc_u.str.str);
        else
          fprintf(f, "%s %d \"%s\"",  nemonic_to_str(bytecode[i][j].nemonic),
                 bytecode[i][j].bc_u.str.dst,
                 encode_str(bytecode[i][j].bc_u.str.str));
	break;
      case REGEXP:
        if (output_format == OUTPUT_SEXPR)
          fprintf(f, "%s %d %d \"%s\"", nemonic_to_str(bytecode[i][j].nemonic),
                 bytecode[i][j].bc_u.regexp.dst,
                 bytecode[i][j].bc_u.regexp.flag,
                 bytecode[i][j].bc_u.regexp.str);
        else
          fprintf(f, "%s %d %d \"%s\"", nemonic_to_str(bytecode[i][j].nemonic),
                 bytecode[i][j].bc_u.regexp.dst,
                 bytecode[i][j].bc_u.regexp.flag,
                 encode_str(bytecode[i][j].bc_u.regexp.str));
	break;
      case CONST: 
      case SPECCONST:
	fprintf(f, "%s %d %s", nemonic_to_str(bytecode[i][j].nemonic),
	       bytecode[i][j].bc_u.cons.dst, 
	       const_to_str(bytecode[i][j].bc_u.cons.cons));
	break;
      case JUMPTRUE: 
      case JUMPFALSE: 
	if (bytecode[i][j].flag == 1) {
	  fprintf(f, "%s %d L%d", nemonic_to_str(bytecode[i][j].nemonic),
		 bytecode[i][j].bc_u.regnum.r1, bytecode[i][j].bc_u.regnum.n1);
	} else {
	  fprintf(f, "%s %d %d", nemonic_to_str(bytecode[i][j].nemonic),
		 bytecode[i][j].bc_u.regnum.r1, bytecode[i][j].bc_u.regnum.n1);
	}
	break;
      case JUMP: 
      case TRY:
	if (bytecode[i][j].flag == 1) {
	  fprintf(f, "%s L%d", nemonic_to_str(bytecode[i][j].nemonic),
		 bytecode[i][j].bc_u.num.n1);
	} else {
	  fprintf(f, "%s %d", nemonic_to_str(bytecode[i][j].nemonic),
		 bytecode[i][j].bc_u.num.n1);	  
	}
	break;
      case LESSTHAN: 
      case LESSTHANEQUAL: 
      case EQ:
      case EQUAL:
      case GETPROP:  
      case SETPROP: 
      case SETARRAY:
      case INSTANCEOF: 
      case NEXTPROPNAME:
	fprintf(f, "%s %d %d %d", nemonic_to_str(bytecode[i][j].nemonic), 
	       bytecode[i][j].bc_u.trireg.r1, bytecode[i][j].bc_u.trireg.r2, 
	       bytecode[i][j].bc_u.trireg.r3);
	break;
      case GETARG:   
      case GETLOCAL: 
	fprintf(f, "%s %d %d %d", nemonic_to_str(bytecode[i][j].nemonic), 
	       bytecode[i][j].bc_u.var.r1, bytecode[i][j].bc_u.var.n1, 
	       bytecode[i][j].bc_u.var.n2);
	break;
      case SETARG:   
      case SETLOCAL:
	fprintf(f, "%s %d %d %d", nemonic_to_str(bytecode[i][j].nemonic), 
	       bytecode[i][j].bc_u.var.n1, 
	       bytecode[i][j].bc_u.var.n2, bytecode[i][j].bc_u.var.r1);
	break;
      case GETGLOBAL: 
      case SETGLOBAL: 
      case NEW: 
      case TYPEOF:
      case ISUNDEF:
      case ISOBJECT:
      case NOT:
      case MAKEITERATOR:
	fprintf(f, "%s %d %d", nemonic_to_str(bytecode[i][j].nemonic), 
	       bytecode[i][j].bc_u.bireg.r1, bytecode[i][j].bc_u.bireg.r2);
	break;
      case GETA:
      case SETA:
      case GETERR:
      case GETGLOBALOBJ:
      case THROW:
	fprintf(f, "%s %d", nemonic_to_str(bytecode[i][j].nemonic),
	       bytecode[i][j].bc_u.unireg.r1);
	break;
      case MOVE:
	if (bytecode[i][j].flag == 2) {
	  fprintf(f, "%s (fl %d) %d", nemonic_to_str(bytecode[i][j].nemonic), 
		 bytecode[i][j].bc_u.bireg.r1, bytecode[i][j].bc_u.bireg.r2);
	} else {
	  fprintf(f, "%s %d %d", nemonic_to_str(bytecode[i][j].nemonic), 
		 bytecode[i][j].bc_u.bireg.r1, bytecode[i][j].bc_u.bireg.r2);
	}
	break;
      case GETIDX: //2011/05/30追記
	fprintf(f, "%s %d %d", nemonic_to_str(bytecode[i][j].nemonic), 
	       bytecode[i][j].bc_u.bireg.r1, bytecode[i][j].bc_u.bireg.r2);
	break;
      case CALL:     
      case SEND:     
      case TAILCALL: 
      case TAILSEND: 
      case MAKECLOSURE: 
      case NEWSEND:
	fprintf(f, "%s %d %d", nemonic_to_str(bytecode[i][j].nemonic),
	       bytecode[i][j].bc_u.regnum.r1, bytecode[i][j].bc_u.regnum.n1);
	break;
      case RET:
      case NEWARGS:
      case FINALLY:
	fprintf(f, "%s", nemonic_to_str(bytecode[i][j].nemonic)); 
	break;
      case SETFL: 
      case ADDSP: 
      case SUBSP: 
	fprintf(f, "%s %d", nemonic_to_str(bytecode[i][j].nemonic),
	       bytecode[i][j].bc_u.num.n1);	  
	break;
      case UNKNOWN: 
	break;
      }
      if (output_format == OUTPUT_SEXPR) fprintf(f, ")");
      fprintf(f, "\n");
    }
    if (output_format == OUTPUT_SEXPR) fprintf(f, "))\n");
  }
  if (output_format == OUTPUT_SEXPR) fprintf(f, ")\n");
}

void print_tree(JSParseNode * root, int indent) {
  if (root == NULL) {
    return;
  }
  printf("%*s", indent, "");
  if (root->pn_type >= NUM_TOKENS) {
    printf("UNKNOWN");
  }
  else {
    printf("%s starts at line %d, column %d, ends at line %d, column %d, pn_arity %d",
           TOKENS[root->pn_type],
           root->pn_pos.begin.lineno, root->pn_pos.begin.index,
           root->pn_pos.end.lineno, root->pn_pos.end.index,root->pn_arity);
  }
  printf("\n");
  printf("%*s", indent, "");
  printf("JSOP:%d\n",root->pn_op);

  switch (root->pn_arity) {
  case PN_UNARY:
    print_tree(root->pn_kid, indent + 2);
    break;
  case PN_BINARY:
    print_tree(root->pn_left, indent + 2);
    print_tree(root->pn_right, indent + 2);
    break;
  case PN_TERNARY:
    print_tree(root->pn_kid1, indent + 2);
    print_tree(root->pn_kid2, indent + 2);
    print_tree(root->pn_kid3, indent + 2);
    break;
  case PN_LIST:
    {
      JSParseNode * p;
      for (p = root->pn_head; p != NULL; p = p->pn_next) {
        print_tree(p, indent + 2);
      }
    }
    break;
  case PN_FUNC:
    printf("%*s\n", indent, "");
    JSObject * object = ATOM_TO_OBJECT(root->pn_funAtom);
    //   printf("test\n");
    fflush(stdout);
    JSFunction * function = (JSFunction *) JS_GetPrivate(context, object);
    //    printf("test\n");
    fflush(stdout);

    /* function name */
    if (function->atom) {
      JSAtom * atom = function->atom;
      printf("%p",atom);
      if (ATOM_IS_STRING(atom)) {
        JSString * strings = ATOM_TO_STRING(atom);
        int i;
        printf("%*s", indent, "");
        for (i = 0;i < strings->length ;i++) {
          char c = strings->chars[i];
          printf("%c",c);
        }
        printf("\n");
      }
    }

    /* function parameters */
    //printf("test\n");
    //fflush(stdout);
    JSAtom ** params = malloc(function->nargs * sizeof(JSAtom *));
    int i;
    //printf("test\n");
    //fflush(stdout);

    for (i = 0; i < function->nargs; i++) {
      params[i] = NULL;
    }
    JSScope * scope = OBJ_SCOPE(object);
    JSScopeProperty * scope_property;
    for (scope_property = SCOPE_LAST_PROP(scope);
         scope_property != NULL;
         scope_property = scope_property->parent) {
      if (scope_property->getter != js_GetArgument) {
        continue;
      }
      //    printf("test2\n");
      //    fflush(stdout);
      //params[(uint16) scope_property->shortid] = JSID_TO_ATOM(scope_property->id);
    }
    for (i = 0; i < function->nargs; i++) {
    }
    free(params);
    //    printf("sclen=%d",root->pn_flags);
    print_tree(root->pn_body,indent+2);
    break;
  case PN_NAME:
    {
      JSAtom * atom = root->pn_atom;
      JSString * strings = ATOM_TO_STRING(atom);
      int i;
      printf("%*s", indent, "");
      for (i = 0; i < strings->length; i++) {
        char c = strings->chars[i];
        printf("%c",c);
      }
      printf("\n");
    }
    if (root->pn_expr != NULL) {
      print_tree(root->pn_expr,indent+2);
    }
    break;
  case PN_NULLARY:
    if (root->pn_type == TOK_STRING) {
      JSAtom * atom = root->pn_atom;
      JSString * strings = ATOM_TO_STRING(atom);
      int i;
      printf("%*s", indent, "");
      for (i=0;i < strings->length ;i++) {
        char c = strings->chars[i];
        printf("%c",c);
      }
      printf("\n");
    } else if(root->pn_type == TOK_NUMBER) {
      printf("%*s", indent, "");
      printf("%lf\n",root->pn_dval);
    } else if(root->pn_type == TOK_OBJECT) {
      printf("%*s", indent, "");
      printf("TOK_OBJECT:op=%d", root->pn_op);
      if (root->pn_op == JSOP_REGEXP) {
        JSAtom * atom = root->pn_atom;
        JSString * strings = ATOM_TO_STRING(atom);
        int i;
        printf("%*s", indent, "");
        for (i = 0;i < strings->length ;i++) {
          char c = strings->chars[i];
          printf("%c",c);
        }
        printf("\n");
      }
    }
    break;
  default:
    fprintf(stderr, "Unknown node type\n");
    exit(EXIT_FAILURE);
    break;
  }
}

char *make_filename(char *av, int rw)  // rw == 0 ==> input, rw == 1 ==> output
{
  int n, c;
  static char *dp;
  char *p;

  if ( rw == 1 ) goto L;
  dp = NULL;
  p = filename;
  n = 0;
  while ((c = *av++) != '\0') {
    *p = c;
    if (c == '/') dp = NULL;
    else if (c == '.') dp = p;
    p++;
    if ( ++n >= FLEN ) {
      /* too long file name */
      return NULL;
    }
  }
  if ( p - filename + 5 >= FLEN ) {
      /* too long file name */
      return NULL;
  }
  if (dp == NULL) {
    dp = p;
    strcat(p, ".js");
  }
  return filename;

 L:
  *dp = '\0';
  if (output_format == OUTPUT_SEXPR)
    strcat(dp, ".tbc");
  else
    strcat(dp, ".sbc");
  return filename;
}

//int main(void) {
int main(int argc, char **argv) {
  JSRuntime * runtime;
  JSObject * global;
  JSTokenStream * token_stream;
  JSParseNode * node;
  Environment rho;
  int i;
  Register globalDst;
  int ac;
  FILE *fp;

  ac = 1;
  if (argc >= 2 && strcmp(argv[1], "-S") == 0) {
    output_format = OUTPUT_SEXPR;
    ac = 2;
  }
  if ( ac == argc ) {
    fp = stdin;
    // No filename is specified, use stdin
  } else if ( ac + 1 == argc ) {
    // filename is specfied, use it
    if ( (input_filename = make_filename(argv[ac], 0)) == NULL) {
      fprintf(stderr, "%s: %s: filename too long\n", argv[0], argv[ac]);
      exit(0);
    }
    //printf("input_filename = %s\n", input_filename);
    if ((fp = fopen(input_filename, "r")) == NULL) {
      fprintf(stderr, "%s: %s: ", argv[0], input_filename);
      perror("");
      exit(0);
    }
    if ( (output_filename = make_filename(argv[ac], 1)) == NULL ) {
      fprintf(stderr, "%s: %s: bad filename\n", argv[0], argv[ac]);
      exit(0);
    }
    //printf("output_filename = %s\n", output_filename);
  } else {
    fprintf(stderr, "Usage: %s [-S] js-filename\n", argv[0]);
    exit(0);
  }

  rho = env_empty();
  init_reg_tbl(0);
  memset(bytecode, 0, sizeof(bytecode));
  fl_tbl[0]=100;
  max_func_fl=0;

  runtime = JS_NewRuntime(8L * 1024L * 1024L);
  if (runtime == NULL) {
    fprintf(stderr, "cannot create runtime");
    exit(EXIT_FAILURE);
  }

  context = JS_NewContext(runtime, 8192);
  if (context == NULL) {
    fprintf(stderr, "cannot create context");
    exit(EXIT_FAILURE);
  }

  global = JS_NewObject(context, NULL, NULL, NULL);
  if (global == NULL) {
    fprintf(stderr, "cannot create global object");
    exit(EXIT_FAILURE);
  }

  if (! JS_InitStandardClasses(context, global)) {
    fprintf(stderr, "cannot initialize standard classes");
    exit(EXIT_FAILURE);
  }

  token_stream = js_NewFileTokenStream(context, NULL, fp);
  if (token_stream == NULL) {
    fprintf(stderr, "cannot create token stream from file\n");
    exit(EXIT_FAILURE);
  }

  node = js_ParseTokenStream(context, global, token_stream);
  if (node == NULL) {
    fprintf(stderr, "parse error in file\n");
    exit(EXIT_FAILURE);
  }

  if (fp != stdin) {
    fclose(fp);
  }

  // print_tree(node, 0);
  // print_bytecode(node);
  set_bc_unireg(GETGLOBALOBJ, 0, 1);
  set_bc_num(SETFL, 2, 0);
  globalDst = search_unuse();
  compile_bytecode(node,rho,globalDst, 0, 0);
  set_bc_unireg(SETA, 0, globalDst);
  set_bc_unireg(RET, 0, 0);
  //  printf("touch_highest = %d, fl = %d\n", highest_touch_reg(),
  //         fl_tbl[0] = calc_fl());
  fl_tbl[0] = calc_fl();
  //printf("touch_highest = %d, fl = %d\n", highest_touch_reg(), fl_tbl[0]);
  set_bc_fl(0);
  code_num[0] = curr_code_num;
  for (i = 1; i < curr_func; i++) {
    compile_function(func_tbl[i], i);
    code_num[i] = curr_code_num;
    fl_tbl[curr_code] = calc_fl();
    set_bc_fl(curr_code);
  }

  if (output_filename != NULL) {
    if ((fp = fopen(output_filename, "w")) == NULL) {
      fprintf(stderr, "%s: %s: ", argv[0], output_filename);
      perror("");
      exit(0);
    }
  } else fp = stdout;

  print_bytecode(bytecode, curr_func, fp);

  if (fp != stdout)
    fclose(fp);

  JS_DestroyContext(context);
  JS_DestroyRuntime(runtime);
  return 0;
}

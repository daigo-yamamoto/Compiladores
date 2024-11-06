/****************************************************/
/* File: tiny.y                                     */
/* The TINY Yacc/Bison specification file           */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/
%{
#define YYPARSER /* distinguishes Yacc output from other code files */

#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parse.h"

#define YYSTYPE TreeNode *
static char * savedName; /* for use in assignments */
static int savedLineNo;  /* ditto */
static TreeNode * savedTree; /* stores syntax tree for later return */
static int yylex(void);
int yyerror(char *);

%}

%token ELSE IF INT RETURN VOID WHILE
%token ID NUM
%token PLUS MINUS TIMES OVER LT LTE RT RTE 
             EQ DIF ASSIGN SEMI COL LPAREN RPAREN LBRCKS 
             RBRCKS LCURBR RCURBR
%token ERROR

%% /* Grammar for TINY */

programa            : declaracao_lista { savedTree = $1; }
                    ;
declaracao_lista    : declaracao_lista declaracao { 
                      YYSTYPE t = $1;
                      if (t != NULL) {
                        while (t->sibling != NULL) {
                          t = t->sibling;
                        }
                        t->sibling = $2;
                        $$ = $1;
                      }
                      else $$ = $1;
                    }
                    | declaracao { $$ = $1; }
                    ;
declaracao          : var_declaracao { $$ = $1; }
                    | fun_declaracao { $$ = $1; }
                    //| error { $$ = NULL; }
                    ;
var_declaracao      : tipo_especificador ID SEMI {
                      $$ = $1;
                      $$->child[0] = newIdNode(Variable);
                      $$->child[0]->attr.name = copyString(popId());
                      $$->child[0]->parent = $$;
                      $$->child[0]->lineno = lineno;
                      $$->child[0]->scopeNode = currentScope;
                    }
                    | tipo_especificador ID LBRCKS NUM RBRCKS SEMI {
                      $$ = $1;
                      $$->child[0] = newIdNode(Array);
                      $$->child[0]->attr.name = copyString(popId());
                      $$->child[0]->parent = $$;
                      $$->child[0]->lineno = lineno;
                      $$->child[0]->child[0] = newExpNode(Constant);
                      $$->child[0]->child[0]->attr.val = numValue;
                      $$->child[0]->scopeNode = currentScope;
                    }
                    ;
tipo_especificador  : INT { $$ = newTypeNode(Int); }
                    | VOID { $$ = newTypeNode(Void); }
                    ;
fun_declaracao      : tipo_especificador ID { savedLineNo = lineno; } LPAREN params RPAREN composto_decl {
                      $$ = $1;
                      $$->child[0] = newIdNode(Function);
                      $$->child[0]->attr.name = copyString(popId());
                      $$->child[0]->parent = $$;
                      $$->child[0]->lineno = savedLineNo;
                      $$->child[0]->child[0] = $5;
                      $$->child[0]->child[1] = $7;
                      $$->child[0]->scopeNode = scopeTree; /* all functions are global */
                    }
                    ;
params              : param_lista { $$ = $1; }
                    | VOID { $$ = NULL; }
                    ;
param_lista         : param_lista COL param {
                      YYSTYPE t = $1;
                      if (t != NULL) {
                        while (t->sibling != NULL) {
                          t = t->sibling;
                        }
                        t->sibling = $3;
                        $$ = $1;
                      }
                      else $$ = $1;
                    }
                    | param { $$ = $1; }
                    ;
param               : tipo_especificador ID { 
                      $$ = $1;
                      $$->child[0] = newIdNode(Variable);
                      $$->child[0]->parent = $$;
                      $$->child[0]->lineno = lineno;
                      $$->child[0]->attr.name = copyString(popId());
                      $$->child[0]->scopeNode = currentScope;
                    }
                    | tipo_especificador ID LBRCKS RBRCKS {
                      $$ = $1;
                      $$->child[0] = newIdNode(Array);
                      $$->child[0]->parent = $$;
                      $$->child[0]->lineno = lineno;
                      $$->child[0]->attr.name = copyString(popId());
                      $$->child[0]->scopeNode = currentScope;
                    }
                    ;
composto_decl       : LCURBR local_declaracoes statement_lista RCURBR {
                      YYSTYPE t = $2;
                      if (t != NULL) {
                        while (t->sibling != NULL) {
                          t = t->sibling;
                        }
                        t->sibling = $3;
                        $$ = $2;
                      }
                      else $$ = $3;
                    }
                    ;
local_declaracoes   : local_declaracoes var_declaracao {
                      YYSTYPE t = $1;
                      if (t != NULL) {
                        while (t->sibling != NULL) {
                          t = t->sibling;
                        }
                        t->sibling = $2;
                        $$ = $1;
                      }
                      else $$ = $2;
                    }
                    | %empty { $$ = NULL; }
                    ;
statement_lista     : statement_lista statement { 
                      YYSTYPE t = $1;
                      if (t != NULL) {
                        while (t->sibling != NULL) {
                          t = t->sibling;
                        }
                        t->sibling = $2;
                        $$ = $1;
                      }
                      else $$ = $2;
                    }
                    | %empty { $$ = NULL; }
                    ;
statement           : expressao_decl  { $$ = $1; }
                    | composto_decl   { $$ = $1; }
                    | selecao_decl    { $$ = $1; }
                    | iteracao_decl   { $$ = $1; }
                    | retorno_decl    { $$ = $1; }
                    ;
expressao_decl      : expressao SEMI { $$ = $1; }
                    | SEMI { $$ = NULL; }
                    ;
selecao_decl        : IF LPAREN expressao RPAREN statement { 
                      $$ = newStmtNode(If);
                      $$->child[0] = $3;
                      $$->child[1] = $5;
                    }
                    | IF LPAREN expressao RPAREN statement ELSE statement { 
                      $$ = newStmtNode(If);
                      $$->child[0] = $3;
                      $$->child[1] = $5;
                      $$->child[2] = $7;
                    }
                    ;
iteracao_decl       : WHILE LPAREN expressao RPAREN statement {
                      $$ = newStmtNode(While);
                      $$->child[0] = $3;
                      $$->child[1] = $5;
                    }
                    ;
retorno_decl        : RETURN SEMI { 
                      $$ = newExpNode(Return);
                      $$->type = VoidType;
                    }
                    | RETURN expressao SEMI { 
                      $$ = newExpNode(Return);
                      $$->child[0] = $2;
                      $$->child[0]->parent = $$;
                    }
                    ;
expressao           : var ASSIGN expressao { 
                      $$ = newStmtNode(Assign);
                      $$->child[0] = $1; /* convention: assigned variable is the left child */
                      $$->child[1] = $3;
                      $$->child[0]->parent = $$;
                      $$->child[1]->parent = $$;
                      // $$->type = $$->child[1]->type; /* todo: review this line */
                    }
                    | simples_expressao { $$ = $1; }
                    ;
var                 : ID { 
                      $$ = newIdNode(Variable);
                      $$->attr.name = copyString(popId());
                      $$->lineno = lineno;
                      $$->scopeNode = currentScope;
                    }
                    | ID LBRCKS expressao RBRCKS { 
                      $$ = newIdNode(Array);
                      $$->attr.name = copyString(popId());
                      $$->lineno = lineno;
                      $$->child[0] = $3;
                      $$->scopeNode = currentScope;
                    }
                    ;
simples_expressao   : soma_expressao relacional soma_expressao { 
                      $$ = $2;
                      $$->child[0] = $1;
                      $$->child[1] = $3;
                      $$->child[0]->parent = $$;
                      $$->child[1]->parent = $$;
                    }
                    | soma_expressao { $$ = $1; }
                    ;
relacional          : LTE     { 
                      $$ = newExpNode(Operator);
                      $$->attr.op = LTE;
                    }
                    | LT { 
                      $$ = newExpNode(Operator);
                      $$->attr.op = LT;
                    }
                    | RT { 
                      $$ = newExpNode(Operator);
                      $$->attr.op = RT;
                    }
                    | RTE { 
                      $$ = newExpNode(Operator);
                      $$->attr.op = RTE;
                    }
                    | EQ { 
                      $$ = newExpNode(Operator);
                      $$->attr.op = EQ;
                    }
                    | DIF { 
                      $$ = newExpNode(Operator);
                      $$->attr.op = DIF;
                    }
                    ;
soma_expressao      : soma_expressao soma termo { 
                      $$ = $2;
                      $$->child[0] = $1;
                      $$->child[1] = $3;
                      $$->child[0]->parent = $$;
                      $$->child[1]->parent = $$;
                    }
                    | termo { $$ = $1; }
                    ;
soma                : PLUS { 
                      $$ = newExpNode(Operator);
                      $$->attr.op = PLUS;
                    }
                    | MINUS { 
                      $$ = newExpNode(Operator);
                      $$->attr.op = MINUS;
                    }
                    ;
termo               : termo mult fator { 
                      $$ = $2;
                      $$->child[0] = $1;
                      $$->child[1] = $3;
                      $$->child[0]->parent = $$;
                      $$->child[1]->parent = $$;
                    }
                    | fator { $$ = $1; }
                    ;
mult                : TIMES { 
                      $$ = newExpNode(Operator);
                      $$->attr.op = TIMES;
                    }
                    | OVER { 
                      $$ = newExpNode(Operator);
                      $$->attr.op = OVER;
                    }
                    ;
fator               : LPAREN expressao RPAREN { $$ = $2; }
                    | var { $$ = $1; }
                    | ativacao { $$ = $1; }
                    | NUM { 
                      $$ = newExpNode(Constant);
                      $$->attr.val = numValue;
                      $$->type = IntegerType;
                    }
                    ;
ativacao            : ID LPAREN args RPAREN { 
                      $$ = newIdNode(Function);
                      $$->attr.name = copyString(popId()); 
                      $$->lineno = lineno;
                      $$->child[0] = $3;
                      for (YYSTYPE t = $$->child[0]; t != NULL; t = t->sibling) {
                        t->parent = $$;
                      }
                      $$->scopeNode = currentScope;
                    }
                    ;
args                : arg_lista { $$ = $1; }
                    | %empty { $$ = NULL; }
                    ;
arg_lista           : arg_lista COL expressao {
                      YYSTYPE t = $1;
                      if (t != NULL) {
                        while (t->sibling != NULL) {
                          t = t->sibling;
                        }
                        t->sibling = $3;
                        $$ = $1;
                      }
                      else $$ = $3;
                    }
                    | expressao { $$ = $1; }
                    ;


%%

int yyerror(char * message)
{ pce("Syntax error at line %d: %s\n",lineno,message);
  pce("Current token: ");
  printToken(yychar,tokenString);
  Error = TRUE;
  return 0;
}

/* yylex calls getToken to make Yacc/Bison output
 * compatible with ealier versions of the TINY scanner
 */
static int yylex(void)
{ return getToken(); }

TreeNode * parse(void)
{ yyparse();
  return savedTree;
}


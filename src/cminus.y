/****************************************************/
/* File: cminus.y                                   */
/* Yacc/Bison specification para C- (estilo Louden) */
/****************************************************/
%{
#define YYPARSER /* distingue Yacc output de outros arquivos */

#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parse.h"

/* Yacc/Bison usa YYSTYPE para saber o tipo-semântico: TreeNode* */
#define YYSTYPE TreeNode *

static char * savedName;   /* para uso em atribuições, se precisar */
static int savedLineNo;    /* idem */
static TreeNode * savedTree; /* armazena a AST final para retorno */
static int yylex(void);
int yyerror(char *);
%}

/* Lista de tokens do scanner */
%token ELSE IF INT RETURN VOID WHILE
%token ID NUM
%token PLUS MINUS TIMES OVER LT LTE RT RTE
       EQ DIF ASSIGN SEMI COL LPAREN RPAREN
       LBRCKS RBRCKS LCURBR RCURBR
%token ERROR

%% /* Grammar principal */

/* programa -> lista de declarações */
programa
  : declaracao_lista
    {
      savedTree = $1;
    }
  ;

/* declaracao_lista -> declaracao_lista declaracao | declaracao */
declaracao_lista
  : declaracao_lista declaracao
    {
      /* Encadeia $2 ao final da lista $1 */
      TreeNode *t = $1;
      if (t != NULL) {
        while (t->sibling != NULL)
          t = t->sibling;
        t->sibling = $2;
        $$ = $1;
      }
      else
        $$ = $2;
    }
  | declaracao
    {
      $$ = $1;
    }
  ;

/* declaracao -> var_declaracao | fun_declaracao */
declaracao
  : var_declaracao
    {
      $$ = $1;
    }
  | fun_declaracao
    {
      $$ = $1;
    }
  ;

/* var_declaracao -> tipo_especificador ID SEMI | tipo_especificador ID [NUM] SEMI */
var_declaracao
  : tipo_especificador ID SEMI
    {
      /* Ex.: int x; */
      $$ = $1; /* nó TypeK(Int ou Void) */
      $$->child[0] = newIdNode(Variable);
      $$->child[0]->attr.name = copyString(popId()); /* ID */
      $$->child[0]->parent = $$;
      $$->child[0]->lineno = lineno;
      $$->child[0]->scopeNode = currentScope;
    }
  | tipo_especificador ID LBRCKS NUM RBRCKS SEMI
    {
      /* Ex.: int x[10]; */
      $$ = $1; /* nó TypeK */
      $$->child[0] = newIdNode(Array);
      $$->child[0]->attr.name = copyString(popId()); /* ID */
      $$->child[0]->parent = $$;
      $$->child[0]->lineno = lineno;

      /* Tamanho do array é guardado em child[0] (ExpK=Constant) */
      $$->child[0]->child[0] = newExpNode(Constant);
      $$->child[0]->child[0]->attr.val = numValue; /* valor do NUM */
      $$->child[0]->scopeNode = currentScope;
    }
  ;

/* tipo_especificador -> INT | VOID */
tipo_especificador
  : INT
    {
      $$ = newTypeNode(Int);
    }
  | VOID
    {
      $$ = newTypeNode(Void);
    }
  ;

/* fun_declaracao -> tipo_especificador ID ( params ) composto_decl */
fun_declaracao
  : tipo_especificador ID { savedLineNo = lineno; }
    LPAREN params RPAREN
    composto_decl
    {
      /* nó TypeK para o tipo (int ou void) */
      $$ = $1;

      /* child[0] do TypeK => nó IdK(Function) */
      $$->child[0] = newIdNode(Function);
      $$->child[0]->attr.name = copyString(popId()); /* nome da função */
      $$->child[0]->parent = $$;
      $$->child[0]->lineno = savedLineNo;

      /* Parâmetros ($5) em child[0] do IdK(Function) */
      $$->child[0]->child[0] = $5;

      /* Corpo da função ($7) em child[1] do IdK(Function) */
      $$->child[0]->child[1] = $7; /* esse $7 será StmtK(Compound) */

      /* No C- estilo, cada função é “global” */
      $$->child[0]->scopeNode = scopeTree; 
    }
  ;

/* params -> param_lista | VOID */
params
  : param_lista
    {
      $$ = $1; /* lista encadeada de parâmetros */
    }
  | VOID
    {
      /* Nenhum parâmetro (função void sem params) */
      $$ = NULL;
    }
  ;

/* param_lista -> param_lista , param | param
   (observação: no seu código, usa COL ','; verifique seu scanner)
*/
param_lista
  : param_lista COL param
    {
      TreeNode *t = $1;
      if (t != NULL) {
        while (t->sibling != NULL)
          t = t->sibling;
        t->sibling = $3;
        $$ = $1;
      }
      else
        $$ = $3;
    }
  | param
    {
      $$ = $1;
    }
  ;

/* param -> tipo_especificador ID | tipo_especificador ID [] */
param
  : tipo_especificador ID
    {
      $$ = $1; /* TypeK */
      $$->child[0] = newIdNode(Variable);
      $$->child[0]->parent = $$;
      $$->child[0]->lineno = lineno;
      $$->child[0]->attr.name = copyString(popId());
      $$->child[0]->scopeNode = currentScope;
    }
  | tipo_especificador ID LBRCKS RBRCKS
    {
      $$ = $1;
      $$->child[0] = newIdNode(Array);
      $$->child[0]->parent = $$;
      $$->child[0]->lineno = lineno;
      $$->child[0]->attr.name = copyString(popId());
      $$->child[0]->scopeNode = currentScope;
    }
  ;

/* ================ REGRA IMPORTANTE ================
   composto_decl => { local_declaracoes statement_lista }
   Cria um nó do tipo Compound (StmtK(Compound)).
*/
composto_decl
  : LCURBR local_declaracoes statement_lista RCURBR
    {
      TreeNode *cmpNode = newStmtNode(Compound);
      /* child[0] = lista de declarações locais (var_declaracao) */
      cmpNode->child[0] = $2;
      /* child[1] = lista de statements */
      cmpNode->child[1] = $3;

      $$ = cmpNode;
    }
  ;

/* local_declaracoes -> local_declaracoes var_declaracao | vazio */
local_declaracoes
  : local_declaracoes var_declaracao
    {
      TreeNode *t = $1;
      if (t != NULL) {
        while (t->sibling != NULL)
          t = t->sibling;
        t->sibling = $2;
        $$ = $1;
      }
      else
        $$ = $2;
    }
  | /* vazio */
    {
      $$ = NULL;
    }
  ;

/* statement_lista -> statement_lista statement | vazio */
statement_lista
  : statement_lista statement
    {
      TreeNode *t = $1;
      if (t != NULL) {
        while (t->sibling != NULL)
          t = t->sibling;
        t->sibling = $2;
        $$ = $1;
      }
      else
        $$ = $2;
    }
  | /* vazio */
    {
      $$ = NULL;
    }
  ;

/* statement -> expressao_decl | composto_decl | selecao_decl | iteracao_decl | retorno_decl */
statement
  : expressao_decl
    {
      $$ = $1;
    }
  | composto_decl
    {
      $$ = $1; /* já é StmtK(Compound) */
    }
  | selecao_decl
    {
      $$ = $1;
    }
  | iteracao_decl
    {
      $$ = $1;
    }
  | retorno_decl
    {
      $$ = $1;
    }
  ;

/* expressao_decl -> expressao ; | ; */
expressao_decl
  : expressao SEMI
    {
      $$ = $1; /* a expressão em si como um statement */
    }
  | SEMI
    {
      $$ = NULL; /* statement vazio */
    }
  ;

/* selecao_decl -> if (expressao) statement [ else statement ] */
selecao_decl
  : IF LPAREN expressao RPAREN statement
    {
      $$ = newStmtNode(If);
      $$->child[0] = $3; /* condição */
      $$->child[1] = $5; /* then-stmt */
    }
  | IF LPAREN expressao RPAREN statement ELSE statement
    {
      $$ = newStmtNode(If);
      $$->child[0] = $3; /* condição */
      $$->child[1] = $5; /* then-stmt */
      $$->child[2] = $7; /* else-stmt */
    }
  ;

/* iteracao_decl -> while (expressao) statement */
iteracao_decl
  : WHILE LPAREN expressao RPAREN statement
    {
      $$ = newStmtNode(While);
      $$->child[0] = $3; /* condição */
      $$->child[1] = $5; /* corpo do while */
    }
  ;

/* retorno_decl -> return ; | return expressao ; */
retorno_decl
  : RETURN SEMI
    {
      /* Aqui estamos usando ExpK(Return), 
         mas poderia ser StmtK(Return) se preferir */
      $$ = newExpNode(Return);
      $$->type = VoidType;
    }
  | RETURN expressao SEMI
    {
      $$ = newExpNode(Return);
      $$->child[0] = $2;
      $$->child[0]->parent = $$;
    }
  ;

/* expressao -> var = expressao | simples_expressao */
expressao
  : var ASSIGN expressao
    {
      $$ = newStmtNode(Assign);
      $$->child[0] = $1; /* var */
      $$->child[1] = $3; /* valor atribuído */
      $$->child[0]->parent = $$;
      $$->child[1]->parent = $$;
    }
  | simples_expressao
    {
      $$ = $1;
    }
  ;

/* var -> ID | ID[expressao] */
var
  : ID
    {
      $$ = newIdNode(Variable);
      $$->attr.name = copyString(popId());
      $$->lineno = lineno;
      $$->scopeNode = currentScope;
    }
  | ID LBRCKS expressao RBRCKS
    {
      $$ = newIdNode(Array);
      $$->attr.name = copyString(popId());
      $$->lineno = lineno;
      $$->child[0] = $3; /* índice */
      $$->scopeNode = currentScope;
    }
  ;

/* simples_expressao -> soma_expressao [relop soma_expressao] */
simples_expressao
  : soma_expressao relacional soma_expressao
    {
      $$ = $2; /* operador relacional */
      $$->child[0] = $1;
      $$->child[1] = $3;
      $$->child[0]->parent = $$;
      $$->child[1]->parent = $$;
    }
  | soma_expressao
    {
      $$ = $1;
    }
  ;

/* relacional -> < | <= | > | >= | == | != */
relacional
  : LT
    {
      $$ = newExpNode(Operator);
      $$->attr.op = LT;
    }
  | LTE
    {
      $$ = newExpNode(Operator);
      $$->attr.op = LTE;
    }
  | RT
    {
      $$ = newExpNode(Operator);
      $$->attr.op = RT;
    }
  | RTE
    {
      $$ = newExpNode(Operator);
      $$->attr.op = RTE;
    }
  | EQ
    {
      $$ = newExpNode(Operator);
      $$->attr.op = EQ;
    }
  | DIF
    {
      $$ = newExpNode(Operator);
      $$->attr.op = DIF;
    }
  ;

/* soma_expressao -> soma_expressao soma termo | termo */
soma_expressao
  : soma_expressao soma termo
    {
      $$ = $2; /* operador + ou - */
      $$->child[0] = $1;
      $$->child[1] = $3;
      $$->child[0]->parent = $$;
      $$->child[1]->parent = $$;
    }
  | termo
    {
      $$ = $1;
    }
  ;

/* soma -> + | - */
soma
  : PLUS
    {
      $$ = newExpNode(Operator);
      $$->attr.op = PLUS;
    }
  | MINUS
    {
      $$ = newExpNode(Operator);
      $$->attr.op = MINUS;
    }
  ;

/* termo -> termo mult fator | fator */
termo
  : termo mult fator
    {
      $$ = $2; /* operador * ou / */
      $$->child[0] = $1;
      $$->child[1] = $3;
      $$->child[0]->parent = $$;
      $$->child[1]->parent = $$;
    }
  | fator
    {
      $$ = $1;
    }
  ;

/* mult -> * | / */
mult
  : TIMES
    {
      $$ = newExpNode(Operator);
      $$->attr.op = TIMES;
    }
  | OVER
    {
      $$ = newExpNode(Operator);
      $$->attr.op = OVER;
    }
  ;

/* fator -> (expressao) | var | ativacao | NUM */
fator
  : LPAREN expressao RPAREN
    {
      $$ = $2;
    }
  | var
    {
      $$ = $1;
    }
  | ativacao
    {
      $$ = $1;
    }
  | NUM
    {
      $$ = newExpNode(Constant);
      $$->attr.val = numValue;
      $$->type = IntegerType;
    }
  ;

/* ativacao -> ID ( args ) */
ativacao
  : ID LPAREN args RPAREN
    {
      $$ = newIdNode(Function);
      $$->attr.name = copyString(popId());
      $$->lineno = lineno;
      /* argumentos em child[0] */
      $$->child[0] = $3;
      for (TreeNode *t = $$->child[0]; t != NULL; t = t->sibling)
        t->parent = $$;
      $$->scopeNode = currentScope;
    }
  ;

/* args -> arg_lista | vazio */
args
  : arg_lista
    {
      $$ = $1;
    }
  | /* vazio */
    {
      $$ = NULL;
    }
  ;

/* arg_lista -> arg_lista , expressao | expressao */
arg_lista
  : arg_lista COL expressao
    {
      TreeNode *t = $1;
      if (t != NULL) {
        while (t->sibling != NULL)
          t = t->sibling;
        t->sibling = $3;
        $$ = $1;
      }
      else
        $$ = $3;
    }
  | expressao
    {
      $$ = $1;
    }
  ;

%%

/* Função de erro do parser */
int yyerror(char * message)
{
  pce("Syntax error at line %d: %s\n", lineno, message);
  pce("Current token: ");
  printToken(yychar, tokenString);
  Error = TRUE;
  return 0;
}

/* yylex chama getToken() (scanner) */
static int yylex(void)
{
  return getToken();
}

/* parse() -> chama yyparse() e devolve a AST em savedTree */
TreeNode * parse(void)
{
  yyparse();
  return savedTree;
}

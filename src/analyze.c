/****************************************************/
/* File: analyze.c                                  */
/* Implementação da análise semântica em DUAS PASSADAS
 * e exibição da tabela de símbolos no compilador C-
 ****************************************************/

 #include <stdio.h>
 #include <stdlib.h>
 #include <string.h>
 #include <stddef.h> /* adicionado */
 
 #include "analyze.h"
 #include "globals.h"
 #include "symtab.h"
 #include "util.h"
 #include "log.h"  /* pc(...), pce(...) */
 
 /* Contador de erros semânticos */
 static int semanticErrors = 0;
 
 /* Indica se encontramos "main" em alguma definição de função */
 static int foundMain = 0;
 
 /* Escopo atual (ex: "main", "", "f", etc.) */
 static char currentScopeName[50] = "";
 
 /*--------------------------------------------------*/
 /* Função auxiliar para reportar erro semântico     */
 /*--------------------------------------------------*/
 static void semanticError(int lineno, const char *msgFormat, const char *id)
 {
     pce("Semantic error at line %d: ", lineno);
     pce(msgFormat, id);
     pce("\n");
     semanticErrors++;
 }
 
 /*--------------------------------------------------*/
 /* Insere as funções built-in "input" e "output"    */
 /* na tabela de símbolos, com lineno=0 (sem linhas) */
 /*--------------------------------------------------*/
 static void insertBuiltIns(void)
 {
     /* input() => retorna int, scope="", idType="fun" */
     st_insert("input", 0, "", "fun", "int");
 
     /* output() => retorna void, scope="", idType="fun" */
     st_insert("output", 0, "", "fun", "void");
 }
 
 /*--------------------------------------------------*/
 /* Passada 1: Insere apenas as DECLARAÇÕES          */
 /*--------------------------------------------------*/
 static void insertDecl_pre(TreeNode *t)
 {
     if (t == NULL) return;
 
     if (t->nodekind == IdK)
     {
         /* Exemplo: se for Function => pai é TypeK => DECLARAÇÃO de função */
         if (t->kind.id == Function)
         {
             if (t->parent && t->parent->nodekind == TypeK)
             {
                 char *name = t->attr.name;
 
                 /* Se o TypeK do pai é int ou void */
                 char *dataType = (t->parent->kind.type == Void) ? "void" : "int";
 
                 /* Escopo global = "" */
                 st_insert(name, t->lineno, "", "fun", dataType);
 
                 if (!strcmp(name, "main"))
                     foundMain = 1;
 
                 /* Muda escopo para o nome da função */
                 strcpy(currentScopeName, name);
             }
         }
         /* Se for Variable ou Array => DECLARAÇÃO de variável/array */
         else if (t->kind.id == Variable || t->kind.id == Array)
         {
             /* Só é declaração se o pai for TypeK */
             if (t->parent && t->parent->nodekind == TypeK)
             {
                 char *name = t->attr.name;
                 char *dataType = (t->parent->kind.type == Void) ? "void" : "int";
                 char *idType   = (t->kind.id == Variable) ? "var" : "array";
 
                 if (strcmp(dataType, "void") == 0) {
                     semanticError(t->lineno, "variable declared void", name);
                     return; 
                 }
 
                 /* Verifica se já existe função global com esse nome */
                 if (st_lookup_local(name, "")) {
                     char symbolType[10];
                     strncpy(symbolType, st_symbolType(name, ""), sizeof(symbolType)-1);
                     symbolType[sizeof(symbolType)-1] = 0; 
                     if (symbolType[0] != '\0' && strcmp(symbolType, "fun") == 0) {
                         semanticError(t->lineno, "'%s' was already declared as a function", name);
                         return; // Evita inserir
                     }
                 }
 
                 /* Insere no escopo atual (ex: nome da função) */
                 if (st_insert(name, t->lineno, currentScopeName, idType, dataType)) {
                     /* Se retornar 1 => redeclaração no mesmo escopo */
                     semanticError(t->lineno, "'%s' was already declared as a variable", name);
                 }
             }
         }
     }
 }
 
 /* Ao sair de um nó: se for Function (e de fato definido), restauramos escopo="" */
 static void insertDecl_post(TreeNode *t)
 {
     if (t == NULL) return;
 
     if (t->nodekind == IdK && t->kind.id == Function)
     {
         /* terminou corpo da função => volta para escopo global,
          * mas só se este IdK realmente era declaração */
         if (t->parent && t->parent->nodekind == TypeK)
         {
             strcpy(currentScopeName, "");
         }
     }
 }
 
 /*--------------------------------------------------*/
 /* Passada 2: Insere USOS e checa se declarados     */
 /*--------------------------------------------------*/
 static void insertUse_pre(TreeNode *t)
 {
     if (t == NULL) return;
     if (t->nodekind != IdK) return;
 
     /* 1) Se for Function + pai TypeK => é a DEF de função (já inserida) */
     if (t->kind.id == Function && t->parent && t->parent->nodekind == TypeK)
     {
         strcpy(currentScopeName, t->attr.name);
         return;
     }
 
     /* 2) Se for FunctionCall => uso de função */
     if (t->kind.id == FunctionCall)
     {
         char *name = t->attr.name;
         int foundLocal  = st_lookup_local(name, currentScopeName);
         int foundGlobal = st_lookup_local(name, "");
 
         if (!foundLocal && !foundGlobal)
         {
             semanticError(t->lineno, "'%s' was not declared in this scope", name);
         }
         else
         {
             if (foundLocal)
                 st_insert(name, t->lineno, currentScopeName, NULL, NULL);
             else
                 st_insert(name, t->lineno, "", NULL, NULL);
         }
         return;
     }
 
     /* 3) Se for Variable ou Array => uso de variável (se pai NÃO for TypeK) */
     if ((t->kind.id == Variable || t->kind.id == Array) &&
         !(t->parent && t->parent->nodekind == TypeK))
     {
         char *name = t->attr.name;
         int foundLocal  = st_lookup_local(name, currentScopeName);
         int foundGlobal = st_lookup_local(name, "");
 
         if (!foundLocal && !foundGlobal)
         {
             semanticError(t->lineno, "'%s' was not declared in this scope", name);
         }
         else
         {
             if (foundLocal)
                 st_insert(name, t->lineno, currentScopeName, NULL, NULL);
             else
                 st_insert(name, t->lineno, "", NULL, NULL);
         }
         return;
     }
 
     /* 4) Caso o parser não separe FunctionCall de Function,
        mas percebamos que é chamada => se nodekind=IdK + kind.id==Function
        e pai NÃO é TypeK, é uso de função */
     if (t->kind.id == Function && !(t->parent && t->parent->nodekind == TypeK))
     {
         char *name = t->attr.name;
         int foundLocal  = st_lookup_local(name, currentScopeName);
         int foundGlobal = st_lookup_local(name, "");
 
         if (!foundLocal && !foundGlobal)
         {
             semanticError(t->lineno, "'%s' was not declared in this scope", name);
         }
         else
         {
             if (foundLocal)
                 st_insert(name, t->lineno, currentScopeName, NULL, NULL);
             else
                 st_insert(name, t->lineno, "", NULL, NULL);
         }
     }
 }
 
 static void insertUse_post(TreeNode *t)
 {
     if (t == NULL) return;
 
     /* Se for a definição de função (pai TypeK), saímos do escopo */
     if (t->nodekind == IdK && t->kind.id == Function
         && t->parent && t->parent->nodekind == TypeK)
     {
         strcpy(currentScopeName, "");
     }
 }
 
 /*--------------------------------------------------*/
 /* Percurso genérico na AST (preProc e postProc)    */
 /*--------------------------------------------------*/
 static void traverse(TreeNode *t,
                      void (*preProc)(TreeNode *),
                      void (*postProc)(TreeNode *))
 {
     if (t == NULL) return;
 
     preProc(t);
     for (int i = 0; i < MAXCHILDREN; i++)
         traverse(t->child[i], preProc, postProc);
     postProc(t);
 
     traverse(t->sibling, preProc, postProc);
 }
 
 /* Nop */
 static void nullProc(TreeNode *t)
 {
     (void)t;
 }
 
 /*--------------------------------------------------*/
 /* buildSymtab => 2 passadas + built-ins            */
 /*--------------------------------------------------*/
 void buildSymtab(TreeNode *syntaxTree)
 {
     /* 0) Inicializa TS e insere funções nativas */
     st_init();
     insertBuiltIns();
 
     /* 1) Primeira passada: só DECLARAÇÕES */
     traverse(syntaxTree, insertDecl_pre, insertDecl_post);
 
     /* 2) Segunda passada: USOS (e valida declarações) */
     traverse(syntaxTree, insertUse_pre, insertUse_post);
 
     /* Se não achamos main, gera erro */
     if (!foundMain)
     {
         pce("Semantic error: undefined reference to 'main'\n");
         semanticErrors++;
     }
 
     /* Imprime a TS no final */
     pc("\nSymbol table:\n\n");
     printSymTab();
 }
 
 /*--------------------------------------------------*/
 /* Se quiser verificação de tipos, faça aqui        */
 /*--------------------------------------------------*/
 
 /*
    Exemplo de checagem para "invalid use of void expression":
 
    Se a função é void, mas está sendo usada em um contexto 
    que espera valor (por ex: a = funcVoid(); ), geramos erro.
 */
 static void checkNode(TreeNode *t)
 {
     /* Verifica se este nó é uma chamada (FunctionCall) OU 
        é um Function usado como chamada (pai não é TypeK). */
     if (t->nodekind == IdK)
     {
         int isFunctionCallNode =
             (t->kind.id == FunctionCall)
             || (t->kind.id == Function && !(t->parent && t->parent->nodekind == TypeK));
 
         if (isFunctionCallNode)
         {
             char *retType = st_dataType(t->attr.name, currentScopeName);
             if (retType != NULL && strcmp(retType, "void") == 0)
             {
                 /* Se o pai for algo que indica "uso em expressão" */
                 TreeNode *p = t->parent;
                 if (p != NULL)
                 {
                     /*
                       Se o pai for IdK ou ExpK=Operator ou StmtK=Assign,
                       consideramos que está em contexto que espera valor.
                     */
                     if (p->nodekind == IdK
                         || (p->nodekind == ExpK && p->kind.exp == Operator)
                         || (p->nodekind == StmtK && p->kind.stmt == Assign))
                     {
                         semanticError(t->lineno, "invalid use of void expression", t->attr.name);
                     }
                 }
             }
         }
     }
 }
 
 void typeCheck(TreeNode *syntaxTree)
 {
     traverse(syntaxTree, nullProc, checkNode);
     /* Se quiser, pode imprimir total de erros no final, etc. */
     if (semanticErrors > 0)
     {
         // pce("Type check found %d semantic errors.\n", semanticErrors);
     }
 }
 
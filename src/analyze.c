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
 
 /* Escopo atual (por ex: "main", "f", "g", etc.) */
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
        /* 1) Declaração de função (kind.id == Function).
         *    Só ocorre se o pai for TypeK => é realmente declaração/definição. */
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
        /* 2) Declaração de variável ou array */
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
                    return; /* Não insere variável void na tabela de símbolos */
                }

                /* **NEW CHECK:** Check if function with same name exists in global scope */
                if (st_lookup_local(name, "")) { // Check global scope ("")
                    char symbolType[4];
                    strncpy(symbolType, st_symbolType(name, ""), sizeof(symbolType)-1);
                    symbolType[sizeof(symbolType)-1] = 0; // Ensure null termination
                    if (strcmp(symbolType, "fun") == 0) {
                        semanticError(t->lineno, "'%s' was already declared as a function", name);
                        return; // Don't insert variable if function name conflict
                    }
                }

                /* Insere no escopo atual (função ou global) */
                if (st_insert(name, t->lineno, currentScopeName, idType, dataType)) {
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
          * mas só se este IdK realmente era declaração (pai TypeK) */
         if (t->parent && t->parent->nodekind == TypeK)
         {
             strcpy(currentScopeName, "");
         }
     }
 }
 
 /*--------------------------------------------------*/
 /* Passada 2: Insere USOS e checa se declarados     */
 /*--------------------------------------------------*/
 /*
    A diferença aqui é que, ao registrar um uso,
    buscamos primeiro no escopo local e, se não acharmos,
    buscamos no global. Só então chamamos st_insert
    com o escopo CORRETO (onde ele foi achado).
 */
 static void insertUse_pre(TreeNode *t)
 {
     if (t == NULL) return;
     if (t->nodekind != IdK) return;
 
     /* 1) Se for Function e o pai for TypeK => DEFINIÇÃO novamente (já inserida) */
     if (t->kind.id == Function && t->parent && t->parent->nodekind == TypeK)
     {
         /* Entramos no escopo dela */
         strcpy(currentScopeName, t->attr.name);
         return;
     }
 
     /* 2) Se for FunctionCall => uso de função */
     if (t->kind.id == FunctionCall)
     {
         char *name = t->attr.name;
 
         /* Tenta local (caso admitamos função local) e global */
         int foundLocal  = st_lookup_local(name, currentScopeName);
         int foundGlobal = st_lookup_local(name, ""); /* escopo global */
 
         if (!foundLocal && !foundGlobal)
         {
             semanticError(t->lineno, "'%s' was not declared in this scope", name);
         }
         else
         {
             /* se achou local, registra uso no local.
                caso contrário, registra uso no global */
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
        mas aqui percebamos que é chamada => heurística:
        Se nodekind=IdK e kind.id=Function, mas pai NÃO é TypeK,
        provavelmente é chamada => uso de função. */
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
         /* não altera currentScopeName */
     }
 }
 
 /* Ao sair de um nó na segunda passada */
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
 static void checkNode(TreeNode *t)
 {
     (void)t;
     /* implementar regras de coerência de tipos se desejar */
 }
 
 void typeCheck(TreeNode *syntaxTree)
 {
     traverse(syntaxTree, nullProc, checkNode);
     /* poderia exibir se houve 'semanticErrors' etc. */
 }
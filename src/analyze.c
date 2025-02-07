/****************************************************/
/* File: analyze.c                                  */
/* Implementação da análise semântica em DUAS PASSADAS
 * e exibição da tabela de símbolos no compilador C- 
 ****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

    /* output() => retorna void, scope="" */
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
         *    Só ocorre se o pai for TypeK, indicando
         *    realmente uma definição/declaração de função. */
        if (t->kind.id == Function)
        {
            if (t->parent && t->parent->nodekind == TypeK)
            {
                /* Recupera nome da função */
                char *name = t->attr.name;

                /* Verifica se o pai (TypeK) é int ou void */
                char *dataType = "int";
                if (t->parent->kind.type == Void)
                    dataType = "void";
                else
                    dataType = "int";

                /* Escopo global = "" */
                st_insert(name, t->lineno, "", "fun", dataType);

                if (!strcmp(name, "main"))
                    foundMain = 1;

                /* Muda escopo p/ o nome da função */
                strcpy(currentScopeName, name);
            }
        }
        /* 2) Declaração de variável ou array */
        else if (t->kind.id == Variable || t->kind.id == Array)
        {
            /* Só é DECLARAÇÃO se o pai for TypeK */
            if (t->parent && t->parent->nodekind == TypeK)
            {
                char *name = t->attr.name;
                char *dataType = (t->parent->kind.type == Void) ? "void" : "int";
                char *idType   = (t->kind.id == Variable) ? "var" : "array";

                /* Insere no escopo atual (função ou global) */
                st_insert(name, t->lineno, currentScopeName, idType, dataType);
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
        /* terminou corpo da função => volta p/ global,
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
static void insertUse_pre(TreeNode *t)
{
    if (t == NULL) return;
    if (t->nodekind != IdK) return;

    /* 1) Se for Function e o pai for TypeK => DEFINIÇÃO (de novo) */
    if (t->kind.id == Function && t->parent && t->parent->nodekind == TypeK)
    {
        /* É a definição de função (já inserida). Entramos no escopo dela */
        strcpy(currentScopeName, t->attr.name);
        return;
    }

    /* 2) Se for FunctionCall => uso de função */
    if (t->kind.id == FunctionCall)
    {
        char *name = t->attr.name;
        /* Funções sempre estão no escopo global */
        int found = st_lookup(name, "");
        if (!found)
        {
            semanticError(t->lineno, "'%s' was not declared in this scope", name);
            /* Não faz st_insert para não "inventar" a função */
        }
        else
        {
            /* Registra o uso (não sobrescreve tipo) => (NULL, NULL) */
            st_insert(name, t->lineno, "", NULL, NULL);
        }
        return;
    }

    /* 3) Se for Variable ou Array => uso de variável (se pai não é TypeK) */
    if (t->kind.id == Variable || t->kind.id == Array)
    {
        /* Se o pai NÃO for TypeK => USO */
        if (!(t->parent && t->parent->nodekind == TypeK))
        {
            char *name = t->attr.name;
            int found = st_lookup(name, currentScopeName);
            if (!found)
            {
                /* não encontrado nem no escopo atual nem global => erro */
                semanticError(t->lineno, "'%s' was not declared in this scope", name);
            }
            else
            {
                /* registra linha de uso => (NULL, NULL) */
                st_insert(name, t->lineno, currentScopeName, NULL, NULL);
            }
        }
        return;
    }

    /* 4) Caso seu parser não separe FunctionCall de Function,
     *    mas aqui percebamos que é chamada => heurística:
     *    Se nodekind=IdK e kind.id=Function, mas pai NÃO é TypeK,
     *    provavelmente é chamada => uso de função. */
    if (t->kind.id == Function)
    {
        /* Se pai não é TypeK => deve ser chamada => uso */
        if (!(t->parent && t->parent->nodekind == TypeK))
        {
            char *name = t->attr.name;
            int found = st_lookup(name, "");
            if (!found)
            {
                semanticError(t->lineno, "'%s' was not declared in this scope", name);
            }
            else
            {
                st_insert(name, t->lineno, "", NULL, NULL);
            }
            /* não altera currentScopeName */
        }
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

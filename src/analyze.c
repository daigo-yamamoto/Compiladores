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

/* Se encontramos "main" */
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
/* na tabela de símbolos com lineno=0               */
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
    switch (t->nodekind)
    {
    case IdK:
        /* Declaração de função */
        if (t->kind.id == Function)
        {
            /* Descobrir se é "int" ou "void" (pai == TypeK) */
            char *name = t->attr.name;
            char *dataType = "int"; 
            if (t->parent && t->parent->nodekind == TypeK)
            {
                if (t->parent->kind.type == Void) 
                    dataType = "void";
                else
                    dataType = "int";
            }

            /* Funções vão no escopo global => scope="" */
            st_insert(name, t->lineno, "", "fun", dataType);

            /* Se for "main", marca que encontramos */
            if (!strcmp(name, "main")) 
                foundMain = 1;

            /* Muda escopo para o nome da função (para inserir params/vars) */
            strcpy(currentScopeName, name);
        }
        /* Declaração de variável ou array */
        else if (t->kind.id == Variable || t->kind.id == Array)
        {
            /* Só é DECLARAÇÃO se o pai for TypeK */
            if (t->parent && t->parent->nodekind == TypeK)
            {
                char *name = t->attr.name;
                char *dataType = (t->parent->kind.type == Void) ? "void" : "int";
                char *idType   = (t->kind.id == Variable) ? "var" : "array";

                st_insert(name, t->lineno, currentScopeName, idType, dataType);
            }
            /* Se pai não é TypeK, então isso é USO, mas a gente ignora na 1a passada */
        }
        break;
    default:
        break;
    }
}

/* Ao sair de um nó: se for Function, restauramos escopo="" */
static void insertDecl_post(TreeNode *t)
{
    if (t->nodekind == IdK && t->kind.id == Function)
    {
        /* Terminamos a declaração (e corpo) da função. Volta escopo p/ global */
        strcpy(currentScopeName, "");
    }
}

/*--------------------------------------------------*/
/* Passada 2: Insere USOS e checa se foram declarados */
/*--------------------------------------------------*/

/* Pré-ordem da segunda passada: analisa usos */
static void insertUse_pre(TreeNode *t)
{
    /* Só queremos nós do tipo IdK */
    if (t->nodekind != IdK) return;

    /* 1) Se for Function e o pai for TypeK => DEFINIÇÃO (de novo) */
    if (t->kind.id == Function && t->parent && t->parent->nodekind == TypeK)
    {
        /* É a definição de função; então entramos no escopo dela */
        strcpy(currentScopeName, t->attr.name);
        return;
    }

    /* 2) Se for FunctionCall => uso de função no escopo global */
    if (t->kind.id == FunctionCall)
    {
        char *name = t->attr.name;
        int found = st_lookup(name, "");
        if (!found)
        {
            semanticError(t->lineno, "'%s' was not declared in this scope", name);
        }
        else
        {
            /* Registra uso da função */
            st_insert(name, t->lineno, "", NULL, NULL);
        }
        return;
    }

    /* 3) Se for Variable ou Array => pode ser uso (se pai não for TypeK) */
    if (t->kind.id == Variable || t->kind.id == Array)
    {
        /* Checa se o pai é TypeK => então é DECLARAÇÃO (já tratada na 1a passada). */
        if (t->parent && t->parent->nodekind == TypeK)
        {
            /* Então é declaração, não uso -> não faz nada aqui */
            return;
        }
        else
        {
            /* Uso de variável */
            char *name = t->attr.name;
            int found = st_lookup(name, currentScopeName);
            if (!found)
            {
                semanticError(t->lineno, "'%s' was not declared in this scope", name);
            }
            else
            {
                /* Insere uso => idType = NULL */
                st_insert(name, t->lineno, currentScopeName, NULL, NULL);
            }
        }
        return;
    }

    /* 4) Se for Function mas pai NÃO é TypeK => deve ser chamada de função? 
     *    Mas idealmente, você teria kind.id == FunctionCall para isso.
     *    Então, se chegou aqui, pode ser algum caso de AST que não diferencia.
     *    Se seu parser não separa Function/FunctionCall, ative esse "plano B": */

    if (t->kind.id == Function)
    {
        /* Se não for pai TypeK, assumimos que é chamada */
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
        /* Não altera currentScopeName aqui, pois é uso, não definição */
        return;
    }
}

/* Pós-ordem da segunda passada */
static void insertUse_post(TreeNode *t)
{
    /* Se for Definition de função novamente, saímos do escopo */
    if (t->nodekind == IdK && t->kind.id == Function
        && t->parent && t->parent->nodekind == TypeK)
    {
        strcpy(currentScopeName, "");
    }
}

/*--------------------------------------------------*/
/* Percorre a AST chamando preProc e postProc       */
/*--------------------------------------------------*/
static void traverse(TreeNode *t,
                     void (*preProc)(TreeNode *),
                     void (*postProc)(TreeNode *))
{
    if (t == NULL) return;

    preProc(t);
    for (int i=0; i<MAXCHILDREN; i++)
        traverse(t->child[i], preProc, postProc);
    postProc(t);

    /* Sibling */
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
    /* 0) Zera TS e insere funções nativas */
    st_init();
    insertBuiltIns();

    /* 1) Primeira passada: DECLARAÇÕES */
    traverse(syntaxTree, insertDecl_pre, insertDecl_post);

    /* 2) Segunda passada: USOS + checa se foram declarados */
    traverse(syntaxTree, insertUse_pre, insertUse_post);

    /* Se não achamos main, erro */
    if (!foundMain)
    {
        pce("Semantic error: undefined reference to 'main'\n");
        semanticErrors++;
    }

    /* Ao final, imprime a tabela de símbolos */
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

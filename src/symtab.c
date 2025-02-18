/****************************************************/
/* File: symtab.c                                   */
/* Implementação da tabela de símbolos (hash)       */
/* mantendo ordem de inserção                       */
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h> /* adicionado */

#include "symtab.h"
#include "util.h"
#include "globals.h"
#include "log.h"  /* para pc(...) e pce(...) */

#define SIZE 211    /* tamanho da hash */
#define SHIFT 4     /* para função hash */

/* Cada nó de "LineList" guarda uma linha em que o símbolo aparece */
typedef struct LineListRec
{
    int lineno;
    struct LineListRec *next;
} *LineList;

/* A "BucketList" representa cada símbolo guardado na TS */
typedef struct BucketListRec
{
    char *name;
    char *scope;           /* ex: "main", "" (global) */
    char *idType;          /* "fun", "var", "array" */
    char *dataType;        /* "int", "void", etc. */
    LineList lines;
    struct BucketListRec *next;
} *BucketList;

/* Tabela de símbolos global, implementada como hash */
static BucketList hashTable[SIZE];

/* Vetor para manter a ordem de inserção */
static BucketList symbolArray[1000];
static int symbolCount = 0;

/*---------------------------------------------*/
/* Função hash: mapeia string -> índice        */
/*---------------------------------------------*/
static int hash(const char *key)
{
    unsigned int temp = 0;
    int i = 0;
    while (key[i] != '\0')
    {
        temp = ((temp << SHIFT) + key[i]) % SIZE;
        ++i;
    }
    return temp;
}

/*---------------------------------------------*/
/* Compara se b corresponde a (name, scope)    */
/*---------------------------------------------*/
static int sameNameScope(BucketList b, const char *name, const char *scope)
{
    if (!b) return 0;
    if (strcmp(b->name, name) != 0) return 0;
    return (strcmp(b->scope, scope) == 0);
}

/*---------------------------------------------*/
/* Inicializa a Tabela de Símbolos             */
/*---------------------------------------------*/
void st_init(void)
{
    for (int i = 0; i < SIZE; i++)
        hashTable[i] = NULL;
    symbolCount = 0;
}

/*-------------------------------------------------------*/
/* Verifica se 'head' já contém 'lineno' para não        */
/* duplicar linha na lista de linhas                     */
/*-------------------------------------------------------*/
static int alreadyHasLine(LineList head, int lineno)
{
    for (LineList p = head; p != NULL; p = p->next)
    {
        if (p->lineno == lineno)
            return 1;
    }
    return 0;
}

/*-------------------------------------------------------*/
/* Cria e retorna um novo BucketList (símbolo)           */
/*-------------------------------------------------------*/
static BucketList newBucket(const char *name, const char *scope,
                            const char *idType, const char *dataType,
                            int lineno)
{
    BucketList newB = (BucketList)malloc(sizeof(*newB));
    newB->name     = copyString(name);
    newB->scope    = copyString(scope);
    newB->idType   = (idType)   ? copyString(idType) : NULL;
    newB->dataType = (dataType) ? copyString(dataType) : NULL;
    newB->next     = NULL;

    if (lineno != 0)
    {
        LineList ll  = (LineList)malloc(sizeof(*ll));
        ll->lineno   = lineno;
        ll->next     = NULL;
        newB->lines  = ll;
    }
    else
    {
        newB->lines = NULL;
    }

    return newB;
}

/*-------------------------------------------------------*/
/* st_insert: Insere (ou atualiza) um símbolo na TS      */
/*  - Se 'idType' != NULL => é DECLARAÇÃO => atualiza    */
/*  - Se 'idType' == NULL => é USO => só adiciona linha  */
/* Retorna 0 se inseriu novo/atualizou uso, 1 se redecl  */
/*-------------------------------------------------------*/
int st_insert(const char *name, int lineno,
              const char *scope,
              const char *idType,
              const char *dataType)
{
    int h = hash(name);
    BucketList l = hashTable[h];
    BucketList prev = NULL;

    /* Tenta achar (name, scope) na lista ligada */
    while (l != NULL && !sameNameScope(l, name, scope))
    {
        prev = l;
        l = l->next;
    }

    if (l == NULL)
    {
        /* Não achou => cria um novo bucket e adiciona */
        BucketList newB = newBucket(name, scope, idType, dataType, lineno);

        if (prev == NULL)
            hashTable[h] = newB;
        else
            prev->next = newB;

        /* Salva no array para impressão na ordem de inserção */
        symbolArray[symbolCount++] = newB;
        return 0; /* Inseriu novo */
    }
    else
    {
        /* Já existe no escopo: possivelmente atualiza ou reporta redeclaração */
        if (idType != NULL)
        {
            /* Achou declaração já existente no escopo atual => redeclaração */
            return 1; 
        }
        else
        {
            /* É uso => só adiciona linha de uso, se ainda não existir */
            if (lineno != 0 && !alreadyHasLine(l->lines, lineno))
            {
                if (l->lines == NULL)
                {
                    LineList ll  = (LineList)malloc(sizeof(*ll));
                    ll->lineno   = lineno;
                    ll->next     = NULL;
                    l->lines     = ll;
                }
                else
                {
                    LineList t = l->lines;
                    while (t->next != NULL)
                        t = t->next;
                    LineList newLine = (LineList)malloc(sizeof(*newLine));
                    newLine->lineno  = lineno;
                    newLine->next    = NULL;
                    t->next          = newLine;
                }
            }
            return 0; /* Atualizou uso */
        }
    }
}

/*------------------------------------------------------------*/
/* st_lookup: Retorna 1 se achar 'name' no 'scope' ou global, */
/* senão 0                                                    */
/*------------------------------------------------------------*/
int st_lookup(const char *name, const char *scope)
{
    int h = hash(name);
    BucketList l = hashTable[h];

    /* 1) Tenta achar (name, scope) exato */
    while (l != NULL)
    {
        if (sameNameScope(l, name, scope))
            return 1;
        l = l->next;
    }

    /* 2) Se não achou e scope != "", procura no escopo global ("") */
    if (strcmp(scope, "") != 0)
    {
        h = hash(name);
        l = hashTable[h];
        while (l != NULL)
        {
            if (sameNameScope(l, name, ""))
                return 1;
            l = l->next;
        }
    }
    return 0;
}

/*------------------------------------------------------------*/
/* st_lookup_local: Retorna 1 se achar (name, scope) exato,   */
/* senão 0 (não olha global).                                 */
/*------------------------------------------------------------*/
int st_lookup_local(const char *name, const char *scope)
{
    int h = hash(name);
    BucketList l = hashTable[h];

    while (l != NULL)
    {
        if (sameNameScope(l, name, scope))
            return 1;
        l = l->next;
    }
    return 0;
}

/*------------------------------------------------------------*/
/* st_symbolType: Retorna o idType ("fun","var","array")      */
/* ou NULL se não achar                                       */
/*------------------------------------------------------------*/
char* st_symbolType(const char *name, const char *scope)
{
    int h = hash(name);
    BucketList l = hashTable[h];

    /* 1) Tenta achar (name,scope) exato */
    while (l != NULL)
    {
        if (sameNameScope(l, name, scope))
            return l->idType;
        l = l->next;
    }

    /* 2) Se não achou, tenta no escopo global */
    if (strcmp(scope, "") != 0)
    {
        h = hash(name);
        l = hashTable[h];
        while (l != NULL)
        {
            if (sameNameScope(l, name, ""))
                return l->idType;
            l = l->next;
        }
    }
    return NULL;
}

/*------------------------------------------------------------*/
/* st_dataType: Retorna o dataType ("int","void") ou NULL     */
/* se não achar                                               */
/*------------------------------------------------------------*/
char* st_dataType(const char *name, const char *scope)
{
    int h = hash(name);
    BucketList l = hashTable[h];

    /* 1) Tenta achar (name, scope) exato */
    while (l != NULL)
    {
        if (sameNameScope(l, name, scope))
            return l->dataType;
        l = l->next;
    }

    /* 2) Se não achou, tenta escopo global */
    if (strcmp(scope, "") != 0)
    {
        h = hash(name);
        l = hashTable[h];
        while (l != NULL)
        {
            if (sameNameScope(l, name, ""))
                return l->dataType;
            l = l->next;
        }
    }
    return NULL;
}

/*------------------------------------------------------------*/
/* printSymTab: Imprime a Tabela de Símbolos na ordem         */
/* de inserção (symbolArray[0..symbolCount-1]).               */
/*------------------------------------------------------------*/
void printSymTab(void)
{
    pc("Variable Name  Scope     ID Type  Data Type  Line Numbers\n");
    pc("-------------  --------  -------  ---------  -------------------------\n");

    for (int i = 0; i < symbolCount; i++)
    {
        BucketList b = symbolArray[i];
        char *name = b->name;
        char *scp  = b->scope;
        char *idt  = (b->idType)   ? b->idType   : "";
        char *dt   = (b->dataType) ? b->dataType : "";

        pc("%-14s ", name);
        pc("%-9s ", scp);
        pc("%-8s ", idt);
        pc("%-10s ", dt);

        /* Imprime as linhas onde o símbolo aparece */
        LineList t = b->lines;
        while (t != NULL)
        {
            pc("%2d ", t->lineno);
            t = t->next;
        }
        pc("\n");
    }
}
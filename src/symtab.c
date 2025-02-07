/****************************************************/
/* File: symtab.c                                   */
/* Implementação da tabela de símbolos (hash)       */
/* mantendo ordem de inserção                       */
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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

/* Função hash: mapeia string -> índice [0..SIZE-1] */
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

/* Compara se BucketList b corresponde a (name, scope) */
static int sameNameScope(BucketList b, const char *name, const char *scope)
{
    if (!b) return 0;
    if (strcmp(b->name, name) != 0) return 0;
    if (strcmp(b->scope, scope) == 0) return 1;
    return 0;
}

/* Inicializa a Tabela de Símbolos */
void st_init(void)
{
    for (int i = 0; i < SIZE; i++)
        hashTable[i] = NULL;
    symbolCount = 0;
}

/* Verifica se 'head' já contém 'lineno' (para não duplicar linha) */
static int alreadyHasLine(LineList head, int lineno)
{
    for (LineList p = head; p != NULL; p = p->next)
    {
        if (p->lineno == lineno) 
            return 1;
    }
    return 0;
}

/* Cria um novo Bucket para (name, scope, idType, dataType, lineno) */
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

    /* Cria a lista de linhas apenas se lineno != 0 */
    if (lineno != 0)
    {
        LineList ll  = (LineList)malloc(sizeof(*ll));
        ll->lineno   = lineno;
        ll->next     = NULL;
        newB->lines  = ll;
    }
    else
        newB->lines = NULL;  /* p/ built-ins */

    return newB;
}

/* 
 * Insere (ou atualiza) um símbolo na TS. 
 *  - Se 'idType' != NULL => DECLARAÇÃO => sobrescreve idType/dataType
 *  - Se 'idType' == NULL => USO => só adiciona a linha (caso não exista)
 */
void st_insert(const char *name, int lineno,
               const char *scope,
               const char *idType,
               const char *dataType)
{
    int h = hash(name);
    BucketList l = hashTable[h];
    BucketList prev = NULL;

    /* Tenta achar (name, scope) na lista */
    while (l != NULL && !(sameNameScope(l, name, scope)))
    {
        prev = l;
        l = l->next;
    }

    if (l == NULL)
    {
        /* Não existe => cria bucket */
        BucketList newB = newBucket(name, scope, idType, dataType, lineno);

        /* Insere no encadeamento da hash */
        if (prev == NULL)
            hashTable[h] = newB;
        else
            prev->next = newB;

        /* Registra este novo símbolo no array (ordem de criação) */
        symbolArray[symbolCount++] = newB;
    }
    else
    {
        /* Já existe => se idType != NULL, sobrescreve (DECLARAÇÃO) */
        if (idType != NULL)
        {
            free(l->idType);
            free(l->dataType);
            l->idType   = copyString(idType);
            l->dataType = copyString(dataType);
        }

        /* Se lineno != 0 => uso => adiciona a linha se não duplicada */
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
    }
}

/* 
 * Retorna 1 se encontrar 'name' no 'scope' ou "" (global),
 * senão retorna 0
 */
int st_lookup(const char *name, const char *scope)
{
    int h = hash(name);
    BucketList l = hashTable[h];

    /* 1) Tenta (name, scope) */
    while (l != NULL)
    {
        if (sameNameScope(l, name, scope))
            return 1;
        l = l->next;
    }
    /* 2) Se não achou e scope != "", procura no escopo "" */
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

/* 
 * Imprime a Tabela de Símbolos NA ORDEM EM QUE OS SÍMBOLOS FORAM CRIADOS.
 * Usa 'symbolArray[0..symbolCount-1]'.
 */
void printSymTab(void)
{
    pc("Variable Name  Scope     ID Type  Data Type  Line Numbers\n");
    pc("-------------  --------  -------  ---------  -------------------------\n");

    /* Percorre o array na ordem em que os símbolos foram criados */
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

        /* Se for input ou output, não imprime linhas */
        if (!strcmp(name,"input") || !strcmp(name,"output"))
        {
            // nada
        }
        else
        {
            /* imprime as linhas */
            LineList t = b->lines;
            while (t != NULL)
            {
                pc("%2d ", t->lineno);
                t = t->next;
            }
        }

        pc("\n");
    }
}

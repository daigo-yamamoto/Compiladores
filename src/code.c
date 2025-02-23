/****************************************************/
/* File: code.c                                     */
/* Emissão de código (estilo TM) para o compilador  */
/* (Abordagem 3: sem usar buffer interno)           */
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "code.h"
#include "globals.h"
#include "log.h"  /* Para pc(...) e pce(...) */

/* Variáveis estáticas que controlam o "loc" atual e o maior loc já emitido */
static int emitLoc = 0;      
static int highEmitLoc = 0;  

/******************************************/
/* emitComment: Emite um comentário       */
/******************************************/
void emitComment(const char *c)
{
    /* Usa pc(...) para imprimir no destino desejado (arquivo, terminal, etc.) */
    pc("* %s\n", c);
}

/******************************************/
/* emitRO: Emite instrução reg->reg       */
/******************************************/
void emitRO(const char *op, int r, int s, int t, const char *c)
{
    pc("%3d:  %5s  %d,%d,%d", emitLoc, op, r, s, t);
    if (c != NULL) 
        pc("\t; %s", c);
    pc("\n");

    emitLoc++;
    if (highEmitLoc < emitLoc) 
        highEmitLoc = emitLoc;
}

/******************************************/
/* emitRM: Emite instrução reg->mem       */
/******************************************/
void emitRM(const char *op, int r, int d, int s, const char *c)
{
    pc("%3d:  %5s  %d,%d(%d)", emitLoc, op, r, d, s);
    if (c != NULL) 
        pc("\t; %s", c);
    pc("\n");

    emitLoc++;
    if (highEmitLoc < emitLoc) 
        highEmitLoc = emitLoc;
}

/****************************************************/
/* emitRM_Abs: Emite instrução com endereço absoluto*/
/****************************************************/
void emitRM_Abs(const char *op, int r, int a, const char *c)
{
    /* O endereço absoluto é convertido para relativo (PC-relativo):
       offset = a - (emitLoc + 1) */
    int offset = a - (emitLoc + 1);

    pc("%3d:  %5s  %d,%d(%d)", emitLoc, op, r, offset, PC);
    if (c != NULL) 
        pc("\t; %s", c);
    pc("\n");

    emitLoc++;
    if (highEmitLoc < emitLoc) 
        highEmitLoc = emitLoc;
}

/********************************************/
/* emitSkip: Avança emitLoc em howMany      */
/********************************************/
int emitSkip(int howMany)
{
    int oldLoc = emitLoc;
    emitLoc += howMany;
    if (highEmitLoc < emitLoc) 
        highEmitLoc = emitLoc;
    return oldLoc;
}

/********************************************/
/* emitBackup: volta emitLoc                */
/********************************************/
void emitBackup(int loc)
{
    if (loc > highEmitLoc)
    {
        pce("BUG in emitBackup: loc > highEmitLoc!");
    }
    emitLoc = loc;
}

/********************************************/
/* emitRestore: volta loc p/ highEmitLoc    */
/********************************************/
void emitRestore(void)
{
    emitLoc = highEmitLoc;
}

/****************************************************/
/* File: code.c                                     */
/* Emissão de código (estilo TM)                    */
/* (usando apenas pc(...) e pce(...) )              */
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "code.h"
#include "globals.h"
#include "log.h" /* pc(...), pce(...) */

static int emitLoc = 0;     /* posição atual de emissão */
static int highEmitLoc = 0; /* maior posição já emitida */

/******************************************/
/* emitComment: imprime um comentário     */
/******************************************/
void emitComment(const char *c)
{
    pc("* %s\n", c);
}

/******************************************/
/* emitRO: Emite instrução reg->reg       */
/******************************************/
void emitRO(const char *op, int r, int s, int t, const char *c)
{
    pc("%3d:  %5s  %d,%d,%d", emitLoc, op, r, s, t);
    if (c != NULL) pc("\t%s", c);
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
    if (c != NULL) pc("\t%s", c);
    pc("\n");

    emitLoc++;
    if (highEmitLoc < emitLoc) 
        highEmitLoc = emitLoc;
}

/********************************************/
/* emitRM_Abs: offset = a - (emitLoc+1)     */
/********************************************/
void emitRM_Abs(const char *op, int r, int a, const char *c)
{
    int offset = a - (emitLoc + 1);
    pc("%3d:  %5s  %d,%d(%d)", emitLoc, op, r, offset, PC);
    if (c != NULL) pc("\t%s", c);
    pc("\n");

    emitLoc++;
    if (highEmitLoc < emitLoc) 
        highEmitLoc = emitLoc;
}

/********************************************/
/* emitSkip: avança emitLoc em howMany      */
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
        pce("BUG in emitBackup: loc > highEmitLoc!");
    emitLoc = loc;
}

/********************************************/
/* emitRestore: volta loc p/ highEmitLoc    */
/********************************************/
void emitRestore(void)
{
    emitLoc = highEmitLoc;
}

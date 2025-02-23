/****************************************************/
/* File: cgen.c                                     */
/* Exemplo completo de gerador de código estilo TM  */
/* para C- (baseado no Louden)                      */
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "globals.h"
#include "symtab.h"  /* para consultar offsets etc. se precisar */
#include "code.h"    /* emitRO, emitRM, emitRM_Abs, etc. */
#include "cgen.h"    /* header (se você tiver) com codeGen(...) */
#include "util.h"    /* newStmtNode, etc. */
#include "log.h"     /* pc, pce, se quiser logs */

/* Registradores conforme convenção Louden:
   0 (AC), 1 (AC1), 2 (GP), 3 (result?), 4,5,6,7 etc.
   Você pode adaptar conforme seu design. */
#define FP 5  /* Frame Pointer */
#define GP 2  /* Global Pointer */
#define AC 0
#define AC1 1

/* "localOffset" é o deslocamento local no frame.
   Iniciamos em -2, e para cada var local, decrementa.
   Exemplo do Louden. */
static int localOffset = -2;

/* Protótipos (funções internas): */
static void genProgram(TreeNode *tree);
static void genFunction(TreeNode *funNode);
static void genCompound(TreeNode *compoundNode);

static void genDeclList(TreeNode *declList);
static void genStmtList(TreeNode *stmtList);
static void genStmt(TreeNode *stmtNode);
static void genExp(TreeNode *expNode);

/* Funções auxiliares específicas */
static void genIfStmt(TreeNode *ifNode);
static void genWhileStmt(TreeNode *whileNode);
static void genAssignStmt(TreeNode *assignNode);
static void genReturnExp(TreeNode *returnNode);

/* Para gerar endereço de array local: */
static void genArrayAddress(TreeNode *arrayNode, int reg);

/****************************************************/
/* codeGen: principal (chamado externamente)        */
/****************************************************/
void codeGen(TreeNode *syntaxTree, const char *codefile)
{
    pc("* TINY Compilation to TM Code\n");

    /* Prelude */
    pc("* Standard prelude:\n");
    /* Exemplo do Louden:
       Carregamos registadores e limpamos local 0, etc. */
    emitRM("LD", 6, 0, 0, "load maxaddress from location 0");
    emitRM("LD", GP, 0, 0, "load maxaddress from location 0");
    emitRM("ST", 0, 0, 0, "clear location 0");
    pc("* End of standard prelude.\n");

    /* Gera código para o “program” (lista de funções e declarações globais) */
    genProgram(syntaxTree);

    /* Final */
    pc("* End of execution.\n");
    emitRO("HALT", 0, 0, 0, NULL);
}

/****************************************************/
/* genProgram: percorre a AST “top-level”           */
/****************************************************/
static void genProgram(TreeNode *tree)
{
    while (tree != NULL)
    {
        if (tree->nodekind == TypeK)
        {
            TreeNode *idNode = tree->child[0];
            if (idNode && idNode->nodekind == IdK)
            {
                if (idNode->kind.id == Function)
                {
                    /* Achamos uma função */
                    pc("* -> Init Function (");
                    pc(idNode->attr.name);
                    pc(")\n");
                    genFunction(idNode);
                    pc("* <- End Function\n");
                }
                else if (idNode->kind.id == Array)
                {
                    /* Declaração global de array */
                    pc("* -> declare vector (global)\n");
                    // Se quiser, reservar no GP etc.
                    pc("* <- declare vector\n");
                }
                else if (idNode->kind.id == Variable)
                {
                    /* Declaração global de variável */
                    pc("* -> declare var (global)\n");
                    pc("* <- declare var\n");
                }
            }
        }
        tree = tree->sibling; /* avança irmãos */
    }
}

/****************************************************/
/* genFunction: emite código p/ uma função          */
/*    funNode->child[0] = lista de params           */
/*    funNode->child[1] = corpo (Compound)          */
/****************************************************/
static void genFunction(TreeNode *funNode)
{
    /* Exemplo de prólogo simples:
       Salvamos RA no frame local (-1) e definimos offset local. */
    emitRM("ST", 0, -1, FP, "store return address");
    localOffset = -2; /* reinicia offset local para esta função */

    /* Se child[1] for Compound, gera corpo */
    TreeNode *body = funNode->child[1];
    if (body && body->nodekind == StmtK && body->kind.stmt == Compound)
    {
        genCompound(body);
    }

    /* Epílogo: retomar o chamador. 
       No Louden, ele faz algo assim:
         LDA 1,0(5)   ; save current fp into ac1
         LD  5,0(5)   ; restore old fp
         LD  7,-1(1)  ; jump para RA
    */
    emitRM("LDA", 1, 0, FP, "save current fp into ac1");
    emitRM("LD",  5, 0, FP,  "restore old fp");
    emitRM("LD",  7, -1, 1,  "return to caller");
}

/****************************************************/
/* genCompound: gera código para { declList stmts } */
/****************************************************/
static void genCompound(TreeNode *compoundNode)
{
    /* child[0] = declList, child[1] = stmtList */
    genDeclList(compoundNode->child[0]);
    genStmtList(compoundNode->child[1]);
}

/****************************************************/
/* genDeclList: gera código para declarações locais */
/****************************************************/
static void genDeclList(TreeNode *declList)
{
    while (declList != NULL)
    {
        if (declList->nodekind == TypeK)
        {
            TreeNode *idNode = declList->child[0];
            if (idNode && idNode->nodekind == IdK)
            {
                if (idNode->kind.id == Array)
                {
                    /* Ex.: int a[10].  
                       Precisamos subtrair '10' do localOffset */
                    int size = 1;
                    if (idNode->child[0]
                        && idNode->child[0]->nodekind == ExpK
                        && idNode->child[0]->kind.exp == Constant)
                    {
                        size = idNode->child[0]->attr.val;
                    }

                    pc("* -> declare vector\n");
                    /* Guarda o endereço base no frame local:
                       "LDA 0, localOffset, FP" e "ST 0, localOffset, FP"? */
                    emitRM("LDA", 0, localOffset, FP, "guard addr of vector");
                    emitRM("ST", 0, localOffset, FP, "store addr of vector");

                    /* Reserva 'size' words no frame */
                    localOffset -= size;

                    pc("* <- declare vector\n");
                }
                else if (idNode->kind.id == Variable)
                {
                    pc("* -> declare var\n");
                    /* Reserva 1 word local */
                    localOffset -= 1;
                    pc("* <- declare var\n");
                }
            }
        }
        declList = declList->sibling;
    }
}

/****************************************************/
/* genStmtList: percorre statements                 */
/****************************************************/
static void genStmtList(TreeNode *stmtList)
{
    while (stmtList != NULL)
    {
        genStmt(stmtList);
        stmtList = stmtList->sibling;
    }
}

/****************************************************/
/* genStmt: chama a função correta p/ cada tipo     */
/****************************************************/
static void genStmt(TreeNode *stmtNode)
{
    if (!stmtNode) return;

    switch (stmtNode->kind.stmt)
    {
    case If:
        genIfStmt(stmtNode);
        break;
    case While:
        genWhileStmt(stmtNode);
        break;
    case Assign:
        genAssignStmt(stmtNode);
        break;
    case Compound:
        genCompound(stmtNode);
        break;
    /* Se tiver Return como StmtK, faça aqui:
    case ReturnStmt:
        genReturnStmt(stmtNode);
        break;
    */
    default:
        /* Se você tem Return como ExpK, pode cair em outro lugar */
        emitComment("Unknown statement kind");
        break;
    }
}

/****************************************************/
/* genIfStmt: if (cond) thenStmt [else elseStmt]    */
/****************************************************/
static void genIfStmt(TreeNode *ifNode)
{
    TreeNode *cond  = ifNode->child[0];
    TreeNode *thenPart = ifNode->child[1];
    TreeNode *elsePart = ifNode->child[2];

    emitComment("-> if");

    /* Gera cond no AC(0) */
    genExp(cond);

    /* Salto se cond == 0 */
    int savedLoc1 = emitSkip(1);
    emitComment("if: jump to else (ou end) if false");

    /* then-part */
    genStmt(thenPart);

    /* salto incond se houver else */
    int savedLoc2 = emitSkip(1);
    emitComment("if: jump to end if true");

    /* conserta o salto do 'if false' */
    int elseLoc = emitSkip(0);
    emitBackup(savedLoc1);
    emitRM_Abs("JEQ", 0, elseLoc, "if cond==0 => jump else");
    emitRestore();

    /* else-part */
    if (elsePart != NULL)
    {
        genStmt(elsePart);
    }

    /* conserta salto para “end if” */
    int endLoc = emitSkip(0);
    emitBackup(savedLoc2);
    emitRM_Abs("LDA", PC, endLoc, "jump to end if");
    emitRestore();

    emitComment("<- if");
}

/****************************************************/
/* genWhileStmt: while (cond) { body }              */
/****************************************************/
static void genWhileStmt(TreeNode *whileNode)
{
    emitComment("-> while");

    /* whileNode->child[0] => cond
       whileNode->child[1] => body */
    int startLoc = emitSkip(0); /* label do início do loop */

    /* gera cond => AC(0) */
    genExp(whileNode->child[0]);

    /* se cond==0 => pula para fim do while */
    int savedLoc = emitSkip(1);

    /* corpo do while */
    genStmt(whileNode->child[1]);

    /* volta ao início */
    emitRM_Abs("LDA", PC, startLoc, "go to start of while");

    /* local do fim do while */
    int endLoc = emitSkip(0);
    emitBackup(savedLoc);
    emitRM_Abs("JEQ", 0, endLoc, "break while if cond==0");
    emitRestore();

    emitComment("<- while");
}

/****************************************************/
/* genAssignStmt: var = expr                        */
/****************************************************/
static void genAssignStmt(TreeNode *assignNode)
{
    emitComment("-> assign");

    TreeNode *varNode  = assignNode->child[0];
    TreeNode *exprNode = assignNode->child[1];

    /* Gera a expr no AC(0) */
    genExp(exprNode);

    /* Precisamos saber se var é normal ou array */
    if (varNode->kind.id == Variable)
    {
        /* Supondo offset local. Se você tem st_lookup_offset,
           chame-o. Aqui vamos colocar um offset “fixo” de -2
           só para exemplificar. */
        // Exemplo real:
        // int offset = lookupOffset(varNode->attr.name, varNode->scopeNode);

        int offset = -2; // Exemplo fixo
        if (strcmp(varNode->attr.name, "i")==0) offset = -2;  // Ajuste se quiser

        emitRM("ST", AC, offset, FP, "assign: store value");
    }
    else if (varNode->kind.id == Array)
    {
        /* Gera o endereço do array + índice em AC1, e faz ST 0(1) */
        genArrayAddress(varNode, AC1);
        emitRM("ST", AC, 0, AC1, "assign to array element");
    }

    emitComment("<- assign");
}

/****************************************************/
/* genArrayAddress: carrega em reg o endereço       */
/*    base + índice do arrayNode                    */
/****************************************************/
static void genArrayAddress(TreeNode *arrayNode, int reg)
{
    /* child[0] => expressão do índice */
    TreeNode *indexExp = arrayNode->child[0];

    /* Passo 1: Carregar base do array no 'reg' 
       (exemplo: se array está no offset localOffsetX) */
    // Exemplo fixo:
    int baseOffset = -12; 
    // ou st_lookup_offset(arrayNode->attr.name, arrayNode->scopeNode);

    /* LDA reg, baseOffset(FP) => reg = FP + baseOffset */
    emitRM("LDA", reg, baseOffset, FP, "get base address of array");

    /* Passo 2: gerar indexExp => AC(0) */
    genExp(indexExp);

    /* Passo 3: somar AC(0) ao reg */
    emitRO("ADD", reg, reg, AC, "add index to base");
    /* Se cada elemento fosse 4 bytes, teria que multiplicar indexExp * 4 
       antes. Mas em TM-louden, 1 word = 1 int = 1 offset. */
}

/****************************************************/
/* genReturnExp: se Return for ExpK(Return)         */
/****************************************************/
static void genReturnExp(TreeNode *returnNode)
{
    /* Se Return tem child[0], gera a exp no AC(0).
       Depois move para local -1(??) ou chama epílogo. */
    emitComment("-> return");
    if (returnNode->child[0] != NULL)
        genExp(returnNode->child[0]);

    /* Depois, para “retornar de fato”: 
       LDA 1,0(5)
       LD 5,0(5)
       LD 7,-1(1)
       ou setar algo que o final da function use. 
       Ou se a gente faz um “jump” aqui. 
       Depende do design. */
    emitComment("<- return");
}

/****************************************************/
/* genExp: lida com Operator, Constant, IdK etc.    */
/****************************************************/
static void genExp(TreeNode *expNode)
{
    if (!expNode) return;

    switch (expNode->kind.exp)
    {
    case Operator:
        {
            /* child[0], child[1] */
            TreeNode *left  = expNode->child[0];
            TreeNode *right = expNode->child[1];

            /* Gera left => AC(0) */
            genExp(left);
            /* Empilha left */
            emitRM("ST", AC, localOffset--, FP, "op: push left");
            /* Gera right => AC(0) */
            genExp(right);
            /* Carrega left em AC1 */
            emitRM("LD", AC1, ++localOffset, FP, "op: pop left");
            /* localOffset “volta” (porque empilhou) */

            switch (expNode->attr.op)
            {
            case PLUS:
                emitRO("ADD", AC, AC1, AC, "op +");
                break;
            case MINUS:
                emitRO("SUB", AC, AC1, AC, "op -");
                break;
            case TIMES:
                emitRO("MUL", AC, AC1, AC, "op *");
                break;
            case OVER:
                emitRO("DIV", AC, AC1, AC, "op /");
                break;
            case LT:
                /* Sub => if result < 0 => AC=1, else AC=0 (depende do TM).
                   Precisamos de “SUB AC,AC1,AC” e um “JLT” etc. 
                   Modo simples (estilo Louden):
                   SUB AC,AC1,AC
                   JLT AC,1,PC
                   LDC AC,0
                   LDA PC,1(PC)
                   LDC AC,1
                */
                {
                    int skipLoc = emitSkip(1);
                    emitRO("SUB", AC, AC1, AC, "op <");
                    /* se AC < 0 => true => AC=1, senão=0 */
                    emitRM("JLT", AC, 2, PC, "br if true");
                    emitRM("LDC", AC, 0, 0, "false case");
                    emitRM("LDA", PC, 1, PC, "unconditional jump");
                    int curr = emitSkip(0);
                    emitBackup(skipLoc);
                    emitSkip(1); /* p/ alinhar */
                    emitRestore();
                    emitRM("LDC", AC, 1, 0, "true case");
                }
                break;
            /* E assim por diante para LTE, EQ, DIF, etc. */
            default:
                emitComment("BUG: op não suportado");
                break;
            }
        }
        break;

    case Constant:
        {
            char cBuf[32];
            sprintf(cBuf, "load const %d", expNode->attr.val);
            emitRM("LDC", AC, expNode->attr.val, 0, cBuf);
        }
        break;

    case Return:
        {
            /* Se for ExpK(Return) (sem ser StmtK),
               podemos chamar genReturnExp ou duplicar a lógica. */
            genReturnExp(expNode);
        }
        break;

    default:
        {
            /* Se for IdK (variável ou array) como expressão, 
               precisamos gerar um “LD” do local offset ou do array. */
            if (expNode->nodekind == IdK)
            {
                if (expNode->kind.id == Variable)
                {
                    /* Exemplo fixo: offset -2 => i */
                    int offset = -2;
                    emitRM("LD", AC, offset, FP, "load var");
                }
                else if (expNode->kind.id == Array)
                {
                    /* Precisamos do valor do array[i], 
                       então genArrayAddress, e em seguida LD 0,0(1). */
                    genArrayAddress(expNode, AC1);
                    emitRM("LD", AC, 0, AC1, "load array element");
                }
                else if (expNode->kind.id == Function)
                {
                    /* Chamada de função, se for seu design (FunctionCall).
                       Precisamos de logic para param, call, etc. */
                    emitComment("call function not implemented example");
                }
            }
        }
        break;
    }
}

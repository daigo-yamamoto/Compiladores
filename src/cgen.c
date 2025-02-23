/****************************************************/
/* File: cgen.c                                     */
/* Gerador de código para C- (máquina TM)           */
/* Abordagem 3: escreve direto usando pc(...)       */
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cgen.h"
#include "code.h"
#include "symtab.h"
#include "globals.h"
#include "util.h"
#include "log.h"  /* Para pc(...) e pce(...) */

/* Exemplo de variáveis globais p/ offsets */
static int globalOffset = 0;
static int tmpOffset = 0;

/* Protótipos das funções internas */
static void cGen(TreeNode *tree);
static void genStmt(TreeNode *tree);
static void genExp(TreeNode *tree);

/******************************************/
/* cGen: varre a árvore chamando genStmt/ */
/*       genExp conforme o nodekind.      */
/******************************************/
static void cGen(TreeNode *tree)
{
    while (tree != NULL)
    {
        switch (tree->nodekind)
        {
        case StmtK:
            genStmt(tree);
            break;
        case ExpK:
            genExp(tree);
            break;
        case IdK:
        case TypeK:
            /* Declarações podem ser tratadas aqui, se necessário. */
            break;
        default:
            pce("cGen: nodekind não reconhecido!");
            break;
        }
        tree = tree->sibling;
    }
}

/******************************************/
/* genStmt: Gera código para nós StmtK    */
/******************************************/
static void genStmt(TreeNode *tree)
{
    switch (tree->kind.stmt)
    {
    case If:
        {
            /* if (cond) stmt [else stmt] */
            int savedLoc1, savedLoc2;

            /* 1. Gera código para condição => valor no AC */
            cGen(tree->child[0]);

            /* 2. Se AC == 0, pula para ELSE ( JEQ AC, ??? ) */
            emitRM("JEQ", AC, 1, AC, "if: jump to else (dist=1)");
            savedLoc1 = emitSkip(1); /* reserva 1 instr p/ fixar endereço do else */

            /* 3. then-part */
            cGen(tree->child[1]);

            /* 4. Pular o else-part (LDA PC, ???) */
            savedLoc2 = emitSkip(1);

            /* 5. Preenche o salto do IF (passo 2) */
            emitBackup(savedLoc1);
            emitRM("LDA", PC, 0, PC, "Jump to else part");
            emitRestore();

            /* 6. else-part */
            cGen(tree->child[2]);

            /* 7. Preenche o salto do THEN para o fim do IF */
            emitBackup(savedLoc2);
            emitRM("LDA", PC, 0, PC, "Jump to end of if");
            emitRestore();
        }
        break;

    case Assign:
        {
            /* var = expr */
            TreeNode *varNode = tree->child[0];
            TreeNode *exprNode = tree->child[1];

            /* 1. Gera código p/ expr => resultado no AC */
            cGen(exprNode);

            /* 2. Descobre offset da var (exemplo fixo: -1) */
            int varOffset = -1; 
            if (varNode->kind.id == Variable)
            {
                emitRM("ST", AC, varOffset, GP, "assign: store value");
            }
            else if (varNode->kind.id == Array)
            {
                /* Array: teria que gerar código p/ índice, 
                   somar offset base, etc. (não demonstrado) */
            }
        }
        break;

    case While:
        {
            /* while(cond) stmt */
            int locLoop = emitSkip(0); /* marco do início do loop */

            /* 1. Gera cond => AC */
            cGen(tree->child[0]);

            /* 2. Se cond == 0 => pula fora do loop */
            int savedLoc = emitSkip(1); /* JEQ */

            /* 3. Corpo do while */
            cGen(tree->child[1]);

            /* 4. Volta para o início (locLoop) */
            emitRM_Abs("LDA", PC, locLoop, "while: loop back");

            /* 5. Completa o JEQ para sair do loop */
            int locExit = emitSkip(0);
            emitBackup(savedLoc);
            emitRM_Abs("JEQ", AC, locExit, "while: exit loop");
            emitRestore();
        }
        break;

    default:
        /* Outros nós de statement (return, etc.) podem ser tratados aqui */
        break;
    }
}

/******************************************/
/* genExp: Gera código para nós ExpK      */
/******************************************/
static void genExp(TreeNode *tree)
{
    switch (tree->kind.exp)
    {
    case Operator:
        {
            /* expr op expr */
            TokenType op = tree->attr.op;
            TreeNode *left = tree->child[0];
            TreeNode *right = tree->child[1];

            /* 1. Gera código p/ left => valor em AC */
            cGen(left);

            /* 2. Salva AC na stack temporária */
            emitRM("ST", AC, tmpOffset--, FP, "op: push left");

            /* 3. Gera código p/ right => AC */
            cGen(right);

            /* 4. Recupera o valor de left em AC1 */
            emitRM("LD", AC1, ++tmpOffset, FP, "op: pop left");

            /* 5. Gera instrução da operação */
            switch (op)
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
                {
                    emitRO("SUB", AC, AC1, AC, "op < => AC1-AC");
                    /* se (AC1 - AC) < 0 => true (1), senão false (0) */
                    emitRM("JLT", AC, 2, PC, "jump if true");
                    emitRM("LDC", AC, 0, AC, "false");
                    emitRM("LDA", PC, 1, PC, "uncond jump");
                    emitRM("LDC", AC, 1, AC, "true");
                }
                break;
            /* Caso queira LTE, RT, RTE, EQ, DIF etc. adicione-os */
            default:
                pce("Operador não suportado em genExp.");
                break;
            }
        }
        break;

    case Constant:
        {
            /* Carrega valor imediato em AC */
            emitRM("LDC", AC, tree->attr.val, AC, "load const");
        }
        break;

    case Return:
        {
            /* return expr; (opcional) */
            if (tree->child[0] != NULL)
            {
                cGen(tree->child[0]);
                /* valor de retorno em AC */
            }
            /* Ajuste de PC para voltar da função, etc. (exemplo simplificado) */
            emitRM("LD", PC, -1, FP, "return: load PC from frame");
        }
        break;

    default:
        // Se houver outros tipos de ExpK, trate-os aqui
        break;
    }
}

/******************************************/
/* codeGen: Função principal exportada    */
/******************************************/
void codeGen(TreeNode *syntaxTree, const char *codefile)
{
    /* Apenas um comentário inicial */
    char msg[128];
    sprintf(msg, "Gerando código para '%s'", (codefile ? codefile : "output"));
    emitComment("====================================");
    emitComment(msg);
    emitComment("====================================");

    /* Opcional: código de bootstrap */
    emitComment("Bootstrap: ajustar FP e limpar AC");
    emitRM("LD", FP, 0, GP, "load maxaddress from global area");
    emitRM("ST", AC, 0, AC, "clear");

    /* Gera código percorrendo a syntaxTree */
    cGen(syntaxTree);

    /* Emite HALT no final */
    emitComment("Fim da execucao.");
    emitRO("HALT", 0, 0, 0, NULL);
    emitComment("====================================");
    emitComment("Fim da geracao de codigo.");
}

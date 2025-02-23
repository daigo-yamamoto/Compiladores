/****************************************************/
/* File: cgen.h                                     */
/* Interface do gerador de código para C-           */
/****************************************************/

#ifndef _CGEN_H_
#define _CGEN_H_

#include "globals.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 
 * Gera o código para a árvore sintática 'syntaxTree'
 * e grava no arquivo de código (variável global 'code' em globals.h),
 * cujo nome lógico é codefile (caso queira imprimir no log).
 */
void codeGen(TreeNode *syntaxTree, const char *codefile);

#ifdef __cplusplus
}
#endif

#endif

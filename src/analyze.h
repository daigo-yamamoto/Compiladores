/****************************************************/
/* File: analyze.h                                  */
/* Interface para construção da TS e verificação    */
/* de tipos do compilador C-                        */
/****************************************************/

#ifndef _ANALYZE_H_
#define _ANALYZE_H_

#include "globals.h"

/* Constroi a tabela de símbolos
 * e insere declarações (funções, variáveis, arrays).
 * Também marca usos de cada variável/função.
 * No final, verifica se 'main' foi declarado.
 */
void buildSymtab(TreeNode *syntaxTree);

/* Faz a verificação de tipos,
 * incluindo checar se cada identificador foi declarado.
 * Aqui, se desejar, pode expandir as regras de coerência de tipos.
 */
void typeCheck(TreeNode *syntaxTree);

#endif

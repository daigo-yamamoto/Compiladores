#ifndef _ANALYZE_H_
#define _ANALYZE_H_

#include "globals.h"

/* Constroi a Tabela de Símbolos (2 passadas) */
void buildSymtab(TreeNode *syntaxTree);

/* Verificação de tipos, se desejado */
void typeCheck(TreeNode *syntaxTree);

#endif

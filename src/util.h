#ifndef _UTIL_H_
#define _UTIL_H_

/* Primeiro: globals.h, que declara TreeNode, TokenType, etc. */
#include "globals.h"

/* Agora as funções que usam esses tipos */
void printToken(TokenType token, const char* tokenString);

TreeNode * newStmtNode(StmtKind);
TreeNode * newExpNode(ExpKind);
char * copyString(char *);

/* etc... */
void printTree(TreeNode * );
void printLine(FILE* redundant_source);
TreeNode * newTypeNode(TypeKind type);
TreeNode * newIdNode(IdKind kind);

#endif

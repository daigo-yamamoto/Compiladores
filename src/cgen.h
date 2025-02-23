#ifndef _CGEN_H_
#define _CGEN_H_

#include "globals.h"

#ifdef __cplusplus
extern "C" {
#endif

void codeGen(TreeNode *syntaxTree, const char *codefile);

#ifdef __cplusplus
}
#endif

#endif

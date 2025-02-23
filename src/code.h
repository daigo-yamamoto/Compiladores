/****************************************************/
/* File: code.h                                     */
/* Interface para emissão de código estilo TM       */
/* (só usa pc(...) e pce(...), no estilo Louden)    */
/****************************************************/

#ifndef _CODE_H_
#define _CODE_H_

/* Registradores da “máquina TM” do Louden */
#define AC   0
#define AC1  1
#define GP   6
#define PC   7
#define FP   5

#ifdef __cplusplus
extern "C" {
#endif

/* Emite um comentário (via pc(...)) */
void emitComment(const char *c);

/* Avança o loc de emissão em howMany, retorna loc antigo */
int emitSkip(int howMany);

/* Volta o loc de emissão para 'loc' */
void emitBackup(int loc);

/* Restaura o loc de emissão para o maior loc já emitido */
void emitRestore(void);

/* Emite instrução RO (op r,s,t) - reg->reg */
void emitRO(const char *op, int r, int s, int t, const char *c);

/* Emite instrução RM (op r,d(s)) - reg->mem ou mem->reg */
void emitRM(const char *op, int r, int d, int s, const char *c);

/* Emite instrução com endereço absoluto (offset = a - (emitLoc+1)) */
void emitRM_Abs(const char *op, int r, int a, const char *c);

#ifdef __cplusplus
}
#endif

#endif

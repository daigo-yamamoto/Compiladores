/****************************************************/
/* File: code.h                                     */
/* Interface para emissor de código estilo TM       */
/****************************************************/

#ifndef _CODE_H_
#define _CODE_H_

/* Registradores (máquina TM do Louden, por exemplo) */
#define nREGS 8
#define AC   0
#define AC1  1
#define GP   6
#define PC   7
#define FP   5

#ifdef __cplusplus
extern "C" {
#endif

/* Emite um comentário no arquivo de código (via pc(...)) */
void emitComment(const char *c);

/* Avança o loc de emissão em howMany, retorna loc antigo */
int emitSkip(int howMany);

/* Restaura o loc de emissão para o valor loc */
void emitBackup(int loc);

/* Restaura o loc de emissão para o maior valor já emitido */
void emitRestore(void);

/* Emite instrução de registrador para registrador
 * Formato: op r,s,t
 */
void emitRO(const char *op, int r, int s, int t, const char *c);

/* Emite instrução de registrador para memória
 * Formato: op r,d(s)
 */
void emitRM(const char *op, int r, int d, int s, const char *c);

/* Emite instrução usando endereço absoluto (convertendo para PC-relativo).
 * Formato: op r, address(PC)
 */
void emitRM_Abs(const char *op, int r, int a, const char *c);

#ifdef __cplusplus
}
#endif

#endif

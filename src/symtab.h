#ifndef _SYMTAB_H_
#define _SYMTAB_H_

/* ... suas outras declarações ... */

/* Inicializa a tabela de símbolos */
void st_init(void);

char* st_symbolType(const char *name, const char *scope);

/* Insere (ou atualiza) símbolo */
int st_insert(const char *name, int lineno,
    const char *scope,
    const char *idType,
    const char *dataType);

/* st_lookup: retorna 1 se achar (name, scope) ou global, senão 0 */
int st_lookup(const char *name, const char *scope);

/* st_lookup_local: retorna 1 se achar (name, scope) exato, senão 0 */
int st_lookup_local(const char *name, const char *scope);

/* Imprime a tabela */
void printSymTab(void);

#endif

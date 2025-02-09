/****************************************************/
/* File: symtab.h                                   */
/* Interface para a Tabela de Símbolos (hash)       */
/****************************************************/
#ifndef _SYMTAB_H_
#define _SYMTAB_H_

/* Inicializa a tabela de símbolos */
void st_init(void);

/* Retorna 1 se encontrar 'name' no 'scope' ou escopo global,
   senão 0 */
int st_lookup(const char *name, const char *scope);

/* Retorna 1 se achar (name, scope) exato, senão 0 (não olha global) */
int st_lookup_local(const char *name, const char *scope);

/* Retorna o idType ("fun","var","array") ou NULL se não achar */
char* st_symbolType(const char *name, const char *scope);

/* Retorna o dataType ("int","void") ou NULL se não achar */
char* st_dataType(const char *name, const char *scope);

/* 
  st_insert:
   - Insere (ou atualiza) um símbolo na TS
   - Se 'idType' != NULL => DECLARAÇÃO => tenta inserir;
        se já existe no mesmo escopo, retorna 1 (redeclaração)
        senão retorna 0
   - Se 'idType' == NULL => USO => só insere lineno
*/
int st_insert(const char *name, int lineno,
              const char *scope,
              const char *idType,
              const char *dataType);

/* Imprime a Tabela de Símbolos */
void printSymTab(void);

#endif

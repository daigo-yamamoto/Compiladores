/****************************************************/
/* File: symtab.h                                   */
/* Interface para tabela de símbolos                */
/****************************************************/

#ifndef _SYMTAB_H_
#define _SYMTAB_H_

#include <stdio.h>

/* Inicializa a tabela (zera estruturas) */
void st_init(void);

/* 
 * Insere (ou atualiza) a entrada de 'name' no escopo 'scope'.
 * - 'lineno': linha em que aparece
 * - Se 'idType' e 'dataType' != NULL, é uma DECLARAÇÃO
 *   (por ex: idType="fun", dataType="int")
 * - Se 'idType' == NULL, é apenas um USO -> adiciona linha
 * 
 */
void st_insert(const char *name, int lineno,
               const char *scope,
               const char *idType,
               const char *dataType);

/*
 * Retorna 1 se 'name' foi encontrado no escopo informado
 * ou no escopo global (""), 0 se não encontrado.
 */
int st_lookup(const char *name, const char *scope);

/*
 * Imprime a Tabela de Símbolos com `pc(...)`,
 * no formato:

Symbol table:

Variable Name  Scope     ID Type  Data Type  Line Numbers
-------------  --------  -------  ---------  -------------------------
...
*/
void printSymTab(void);

#endif
    
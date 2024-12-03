/****************************************************/
/* File: globals.h                                  */
/* Global types and vars for TINY compiler          */
/* must come before other include files             */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "log.h"
#include "scopetree.h"

#ifndef YYPARSER
#include "parser.h"
#define ENDFILE 0
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

/* MAXRESERVED = the number of reserved words */
#define MAXRESERVED 8

// typedef enum 
//     /* book-keeping tokens */
//    {ENDFILE,ERROR,NEWLINE,COMMENT,
//     /* reserved words */
//     ELSE, IF, INT, RETURN, VOID, WHILE,
//     /* multicharacter tokens */
//     ID,NUM,
//     /* special symbols */
//     PLUS, MINUS, TIMES, OVER, LT, LTE, RT, RTE, EQ, DIF, ASSIGN, SEMI, COL,
//     LPAREN, RPAREN, LBRCKS, RBRCKS, LCURBR, RCURBR
//    } TokenType;
typedef int TokenType;

extern FILE* source; /* source code text file */
extern FILE* listing; /* listing output text file */
extern FILE* code; /* code text file for TM simulator */
extern FILE* redundant_source;

extern int lineno; /* source line number for listing */
extern ScopeNode *scopeTree; /* scope tree */
extern ScopeNode *currentScope; /* current scope node */

/**************************************************/
/***********   Syntax tree for parsing ************/
/**************************************************/

typedef enum {StmtK, ExpK, IdK, TypeK} NodeKind;
typedef enum {If, Assign, While} StmtKind;
typedef enum {Operator, Constant, Return, FunctionCall} ExpKind;
typedef enum {Variable, Array, Function} IdKind;
typedef enum {Void, Int} TypeKind;

/* ExpType is used for type checking */
typedef enum {VoidType,IntegerType,BooleanType} ExpType;

#define MAXCHILDREN 3

typedef struct treeNode
   { struct treeNode * child[MAXCHILDREN];
     struct treeNode * sibling;
     struct treeNode * parent;
     int lineno;
     ScopeNode *scopeNode;
     NodeKind nodekind;
     union { StmtKind stmt; ExpKind exp; IdKind id; TypeKind type;} kind;
     union { TokenType op;
             int val;
             char * name; } attr;
     ExpType type; /* for type checking of exps */
   } TreeNode;

/**************************************************/
/***********   Flags for tracing       ************/
/**************************************************/

/* EchoSource = TRUE causes the source program to
 * be echoed to the listing file with line numbers
 * during parsing
 */
extern int EchoSource;

/* TraceScan = TRUE causes token information to be
 * printed to the listing file as each token is
 * recognized by the scanner
 */
extern int TraceScan;

/* TraceParse = TRUE causes the syntax tree to be
 * printed to the listing file in linearized form
 * (using indents for children)
 */
extern int TraceParse;

/* TraceAnalyze = TRUE causes symbol table inserts
 * and lookups to be reported to the listing file
 */
extern int TraceAnalyze;

/* TraceCode = TRUE causes comments to be written
 * to the TM code file as code is generated
 */
extern int TraceCode;

/* Error = TRUE prevents further passes if an error occurs */
extern int Error; 
#endif

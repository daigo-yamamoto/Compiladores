#ifndef _SCOPETREE_H
#define _SCOPETREE_H

#include <stdbool.h>
#include <string.h>
#include <stdio.h>

typedef struct scope
{
  char *name;
  int id;
} Scope;

Scope *newScope(char *name, int id);

typedef struct scopeList
{
  Scope *scope;
  struct scopeList *next;
} *ScopeList;

ScopeList newScopeList(char *name, int id);

ScopeList pushScopeList(ScopeList list, char *name, int id);

typedef struct scopeNode
{
  Scope *scope;
  struct scopeNode *parent;
  struct scopeNode **children;
  int numChildren;
} ScopeNode;

ScopeNode *newRootScopeNode();

ScopeNode *newScopeNode(char *name, int id);

ScopeNode *insertScope(ScopeNode *currNode, char *name, int prevId);

bool isInsideScope(ScopeNode *node, ScopeList scopes);

void printScopeTreeNode(char *prefix, ScopeNode *node, bool isLast);

void printScopeTree(ScopeNode *root);

#endif
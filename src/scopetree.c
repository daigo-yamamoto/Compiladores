#include "scopetree.h"
#include <stdlib.h>

Scope *newScope(char *name, int id) {
  Scope *s = (Scope *)malloc(sizeof(Scope));
  s->name = name;
  s->id = id;
  return s;
}

ScopeList newScopeList(char *name, int id) {
  ScopeList list = (ScopeList)malloc(sizeof(struct scopeList));
  list->scope = newScope(name, id);
  list->next = NULL;
  return list;
}

ScopeList pushScopeList(ScopeList list, char *name, int id) {
  ScopeList newNode = (ScopeList)malloc(sizeof(struct scopeList));
  newNode->scope = newScope(name, id);
  newNode->next = NULL;
  ScopeList p = list;
  while (p->next != NULL)
    p = p->next;
  p->next = newNode;
  return list;
}

ScopeNode *newRootScopeNode() {
  ScopeNode *newNode = (ScopeNode *)malloc(sizeof(ScopeNode));
  newNode->scope = newScope("", -1);
  newNode->children = NULL;
  newNode->parent = NULL;
  newNode->numChildren = 0;
  return newNode;
}

ScopeNode *newScopeNode(char *name, int id) {
  ScopeNode *newNode = (ScopeNode *)malloc(sizeof(ScopeNode));
  newNode->scope = newScope(name, id);
  newNode->children = NULL;
  newNode->parent = NULL;
  newNode->numChildren = 0;
  return newNode;
}

ScopeNode *insertScope(ScopeNode *currNode, char *name, int prevId) {
  ScopeNode *newNode = newScopeNode(name, prevId + 1);
  if (currNode->children == NULL) {
    currNode->children = (ScopeNode **)malloc(sizeof(ScopeNode *));
    currNode->children[0] = newNode;
    currNode->children[0]->parent = currNode;
    ++(currNode->numChildren);
  } else {
    currNode->children =
        (ScopeNode **)realloc(currNode->children, (++(currNode->numChildren))*sizeof(ScopeNode*));
    currNode->children[currNode->numChildren - 1] = newNode;
    currNode->children[currNode->numChildren - 1]->parent = currNode;
  }
  return currNode->children[currNode->numChildren - 1];
}

bool isInsideScope(ScopeNode *node, ScopeList scopes) {
  while (node != NULL) // maybe node->parent ?
  {
    for (ScopeList s = scopes; s != NULL; s = s->next) {
      if (node->scope->id == s->scope->id) {
        return true;
      }
    }
    node = node->parent;
  }
  return false;
}

void printScopeTreeNode(char *prefix, ScopeNode *node, bool isLast) {
  if (node == NULL) {
    return;
  }

  printf("%s%s%s#%d\n", prefix, isLast ? "└──" : "├──", node->scope->name,
         node->scope->id);

  char *newprefix = (char *)malloc(1024 * sizeof(char));
  newprefix = strcpy(newprefix, prefix);
  newprefix = strcat(newprefix, isLast ? "    " : "│   ");
  for (int child = 0; child < node->numChildren; ++child) {
    printScopeTreeNode(newprefix, node->children[child],
                       child + 1 == node->numChildren);
  }

  free(newprefix);
}

void printScopeTree(ScopeNode *root) {
  printf("\nScope Tree:\n\n");
  char *prefix = (char *)malloc(1024 * sizeof(char));
  printScopeTreeNode(prefix, root, true);
  free(prefix);
}
/****************************************************/
/* File: util.c                                     */
/* Utility function implementation                  */
/* for the TINY compiler                            */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "util.h"

/* Procedure printToken prints a token 
 * and its lexeme to the listing file
 */
void printToken( TokenType token, const char* tokenString )
{ switch (token)
  { case ELSE:
    case IF:
    case INT:
    case RETURN:
    case VOID:
    case WHILE:
      pc(
         "reserved word: %s\n",tokenString);
      break;
    case ASSIGN: pc("=\n"); break;
    case EQ: pc("==\n"); break;
    case DIF: pc("!=\n"); break;
    case LT: pc("<\n"); break;
    case LTE: pc("<=\n"); break;
    case RT: pc(">\n"); break;
    case RTE: pc(">=\n"); break;
    case LPAREN: pc("(\n"); break;
    case RPAREN: pc(")\n"); break;
    case LBRCKS: pc("[\n"); break;
    case RBRCKS: pc("]\n"); break;
    case LCURBR: pc("{\n"); break;
    case RCURBR: pc("}\n"); break;
    case SEMI: pc(";\n"); break;
    case COL: pc(",\n"); break;
    case PLUS: pc("+\n"); break;
    case MINUS: pc("-\n"); break;
    case TIMES: pc("*\n"); break;
    case OVER: pc("/\n"); break;
    case ENDFILE: pc("EOF\n"); break;
    //case NEWLINE: pc("\n"); break;
    //case COMMENT: pc("\n"); break;
    case NUM:
      pc(
          "NUM, val= %s\n",tokenString);
      break;
    case ID:
      pc(
          "ID, name= %s\n",tokenString);
      break;
    case ERROR:
      pce(
          "ERROR: %s\n",tokenString);
      break;
    default: /* should never happen */
      pce("Unknown token: %d\n",token);
  }
}

/* Function newStmtNode creates a new statement
 * node for syntax tree construction
 */
TreeNode * newStmtNode(StmtKind kind)
{ TreeNode * t = (TreeNode *) malloc(sizeof(TreeNode));
  int i;
  if (t==NULL)
    pce("Out of memory error at line %d\n",lineno);
  else {
    for (i=0;i<MAXCHILDREN;i++) t->child[i] = NULL;
    t->sibling = NULL;
    t->nodekind = StmtK;
    t->kind.stmt = kind;
    t->lineno = lineno;
  }
  return t;
}

/* Function newExpNode creates a new expression 
 * node for syntax tree construction
 */
TreeNode * newExpNode(ExpKind kind)
{ TreeNode * t = (TreeNode *) malloc(sizeof(TreeNode));
  int i;
  if (t==NULL)
    pce("Out of memory error at line %d\n",lineno);
  else {
    for (i=0;i<MAXCHILDREN;i++) t->child[i] = NULL;
    t->sibling = NULL;
    t->nodekind = ExpK;
    t->kind.exp = kind;
    t->lineno = lineno;
    t->type = VoidType;
  }
  return t;
}

/* Function copyString allocates and makes a new
 * copy of an existing string
 */
char * copyString(char * s)
{ int n;
  char * t;
  if (s==NULL) return NULL;
  n = strlen(s)+1;
  t = malloc(n);
  if (t==NULL)
    pce("Out of memory error at line %d\n",lineno);
  else strcpy(t,s);
  return t;
}

/* Variable indentno is used by printTree to
 * store current number of spaces to indent
 */
static int indentno = 0;

/* macros to increase/decrease indentation */
#define INDENT indentno+=2
#define UNINDENT indentno-=2

/* printSpaces indents by printing spaces */
static void printSpaces(void)
{ int i;
  for (i=0;i<indentno;i++)
    pc(" ");
}

void printTree(TreeNode *tree)
{
    int i;
    while (tree != NULL)
    {
        // Imprime o nó atual
        printSpaces();
        if (tree->nodekind == TypeK)
        {
            if (tree->child[0] != NULL && tree->child[0]->nodekind == IdK)
            {
                if (tree->child[0]->kind.id == Function)
                {
                    pc("Declare function (return type \"%s\"): %s\n",
                        tree->kind.type == Void ? "void" : "int",
                        tree->child[0]->attr.name);

                    // Imprime os parâmetros da função
                    if (tree->child[0]->child[0] != NULL)
                    {
                        INDENT;
                        // Percorre a lista de parâmetros
                        TreeNode *param = tree->child[0]->child[0];
                        while (param != NULL)
                        {
                            if (param->nodekind == TypeK && param->child[0] != NULL && param->child[0]->nodekind == IdK)
                            {
                                printSpaces();
                                const char *param_type = (param->kind.type == Int) ? "int" : "void";
                                const char *param_kind;
                                if (param->child[0]->kind.id == Array)
                                    param_kind = "array";
                                else if (param->child[0]->kind.id == Variable)
                                    param_kind = "var";
                                else
                                    param_kind = "unknown";

                                pc("Function param (%s %s): %s\n",
                                    param_type, param_kind,
                                    param->child[0]->attr.name);
                            }
                            param = param->sibling;
                        }
                        UNINDENT;
                    }

                    // Imprime o corpo da função
                    if (tree->child[0]->child[1] != NULL)
                    {
                        INDENT;
                        printTree(tree->child[0]->child[1]);
                        UNINDENT;
                    }
                }
                else if (tree->child[0]->kind.id == Variable)
                {
                    pc("Declare %s var: %s\n",
                        tree->kind.type == Int ? "int" : "void",
                        tree->child[0]->attr.name);
                }
                else if (tree->child[0]->kind.id == Array)
                {
                    pc("Declare %s array: %s\n",
                        tree->kind.type == Int ? "int" : "void",
                        tree->child[0]->attr.name);
                    if (tree->child[0]->child[0] != NULL)
                    {
                        INDENT;
                        printTree(tree->child[0]->child[0]);
                        UNINDENT;
                    }
                }
                else
                {
                    pc("Unknown declaration\n");
                }
            }
            else
            {
                pc("Unknown Type Declaration\n");
            }
        }
        else if (tree->nodekind == StmtK)
        {
            switch (tree->kind.stmt)
            {
            case If:
                pc("Conditional selection\n");
                INDENT;
                if (tree->child[0] != NULL)
                    printTree(tree->child[0]); // condição
                if (tree->child[1] != NULL)
                    printTree(tree->child[1]); // bloco "then"
                if (tree->child[2] != NULL)
                    printTree(tree->child[2]); // bloco "else"
                UNINDENT;
                break;
            case Assign:
                if (tree->child[0] != NULL && tree->child[0]->nodekind == IdK)
                {
                    if (tree->child[0]->kind.id == Variable)
                    {
                        pc("Assign to var: %s\n", tree->child[0]->attr.name);
                    }
                    else if (tree->child[0]->kind.id == Array)
                    {
                        pc("Assign to array: %s\n", tree->child[0]->attr.name);
                    }
                    else
                    {
                        pc("Assign to unknown id: %s\n", tree->child[0]->attr.name);
                    }
                    INDENT;
                    // Imprime índice do array se existir (para arrays)
                    if (tree->child[0]->kind.id == Array && tree->child[0]->child[0] != NULL)
                        printTree(tree->child[0]->child[0]);
                    // Imprime a expressão atribuída
                    if (tree->child[1] != NULL)
                        printTree(tree->child[1]);
                    UNINDENT;
                }
                else
                {
                    pc("Assign\n");
                    INDENT;
                    // Imprime filhos se necessário
                    for (i = 0; i < MAXCHILDREN; i++)
                        if (tree->child[i] != NULL)
                            printTree(tree->child[i]);
                    UNINDENT;
                }
                break;
            case While:
                pc("Iteration (loop)\n");
                INDENT;
                if (tree->child[0] != NULL)
                    printTree(tree->child[0]); // condição do loop
                if (tree->child[1] != NULL)
                    printTree(tree->child[1]); // corpo do loop
                UNINDENT;
                break;
            default:
                pce("Unknown StmtKNode kind\n");
                break;
            }
        }
        else if (tree->nodekind == ExpK)
        {
            switch (tree->kind.exp)
            {
            case Operator:
                pc("Op: ");
                printToken(tree->attr.op, "\0");
                INDENT;
                for (i = 0; i < MAXCHILDREN; i++)
                {
                    if (tree->child[i] != NULL)
                        printTree(tree->child[i]);
                }
                UNINDENT;
                break;
            case Constant:
                pc("Const: %d\n", tree->attr.val);
                break;
            case Return:
                pc("Return\n");
                if (tree->child[0] != NULL)
                {
                    INDENT;
                    printTree(tree->child[0]);
                    UNINDENT;
                }
                break;
            default:
                pce("Unknown ExpKNode kind\n");
                break;
            }
        }
        else if (tree->nodekind == IdK)
        {
            if (tree->kind.id == Variable || tree->kind.id == Array)
            {
                pc("Id: %s\n", tree->attr.name);
                if (tree->kind.id == Array)
                {
                    if (tree->child[0] != NULL)
                    {
                        INDENT;
                        printTree(tree->child[0]);
                        UNINDENT;
                    }
                }
            }
            else if (tree->kind.id == Function)
            {
                pc("Function call: %s\n", tree->attr.name);
                if (tree->child[0] != NULL)
                {
                    INDENT;
                    printTree(tree->child[0]); // Processa os argumentos
                    UNINDENT;
                }
            }
            else
            {
                // Não imprime outros tipos de IdK
            }
        }
        else
        {
            pc("Unknown node kind\n");
        }

        // Move para o próximo irmão
        tree = tree->sibling;
    }
}

/* Procedure printLine prints a full line
 * of the source code, with its number
 * reduntand_source is ANOTHER instance 
 * of file pointer opened with the source code.
 */
void printLine(FILE* redundant_source){
  char line[1024];
  char *ret = fgets(line, 1024, redundant_source);
  // If an error occurs, or if end-of-file is reached and no characters were read, fgets returns NULL.
  if (ret) { pc( "%d: %-1s",lineno, line); 
             // if EOF, the string does not contain \n. add it to separate from EOF token
             if (feof(redundant_source)) pc("\n");
           } 
}

TreeNode *newTypeNode(TypeKind kind)
{
  TreeNode *t = (TreeNode *)malloc(sizeof(TreeNode));
  int i;
  if (t == NULL)
      pce("Out of memory error at line %d\n", lineno);
  else
  {
      for (i = 0; i < MAXCHILDREN; i++)
        t->child[i] = NULL;
      t->sibling = NULL;
      t->parent = NULL;
      t->nodekind = TypeK;
      t->kind.type = kind;
      t->lineno = lineno;
  }
  return t;
}

TreeNode *newIdNode(IdKind kind)
{
  TreeNode *t = (TreeNode *)malloc(sizeof(TreeNode));
  int i;
  if (t == NULL)
    pc("Out of memory error at line %d\n", lineno);
  else
  {
    for (i = 0; i < MAXCHILDREN; i++)
      t->child[i] = NULL;
    t->sibling = NULL;
    t->parent = NULL;
    t->nodekind = IdK;
    t->kind.id = kind;
    t->lineno = lineno;
  }
  return t;
}

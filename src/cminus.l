/****************************************************/
/* File: cminus.l                                   */
/* Lex specification for C-                         */
/* Project for CES41: Compiladores                  */
/****************************************************/

%option noyywrap 
/* opção noyywrap pode ser necessária para novas versões do flex
  limitação: não compila mais de um arquivo fonte de uma só vez (não precisamos disso)
  https://stackoverflow.com/questions/1480138/undefined-reference-to-yylex 
*/ 

%{
#include "globals.h"
#include "util.h"
#include "scan.h"
/* lexeme of identifier or reserved word */
char tokenString[MAXTOKENLEN+1];

int numValue;

/* stores the lexeme of ID tokens */
char* savedId[128];
int savedIdIndex = -1;

void saveId(char *id) {
  savedId[++savedIdIndex] = copyString(id);
}

char *popId() {
  if (savedIdIndex < 0) {
    return "";
  }
  
  return savedId[savedIdIndex--];
}

%}

digit       [0-9]
number      {digit}+
letter      [a-zA-Z]
identifier  {letter}+
newline     \n|(\r\n)
whitespace  [ \t]+
%%
"else"          {return ELSE;}
"if"            {return IF;}
"int"           {return INT;}
"return"        {return RETURN;}
"void"          {return VOID;}
"while"         {return WHILE;}
"+"             {return PLUS;}
"-"             {return MINUS;}
"*"             {return TIMES;}
"/"             {return OVER;}
"<"             {return LT;}
"<="            {return LTE;}
">"             {return RT;}
">="            {return RTE;}
"=="            {return EQ;}
"!="            {return DIF;}
"="             {return ASSIGN;}
";"             {return SEMI;}
","             {return COL;}
"("             {return LPAREN;}
")"             {return RPAREN;}
"["             {return LBRCKS;}
"]"             {return RBRCKS;}
"{"             {return LCURBR;}
"}"             {return RCURBR;}
{number}        {numValue = atoi(yytext); return NUM;}
{identifier}    {saveId(yytext); return ID;}
{newline}       {lineno++; /* skip */}
{whitespace}    {/* skip whitespace */}
"/*"             { char c;
                  int flag1 = 0;
                  int flag2 = 0;
                  do {
                    c = input();
                    if (c == EOF) break;
                    if (c == '\n') lineno++;
                    if (c == '*') flag1 = 1;
                    if (c == '/' && flag1 == 1) flag2 = 1;
                    if (c != '*') flag1 = 0;
                  } while (!flag2);
                  /* skip */ 
                }
.               {return ERROR;}
%%
TokenType getToken(void)
{
    static int firstTime = TRUE;
    static int prev_lineno = 0;
    static int redundant_lineno = 0;
    TokenType currentToken;

    if (firstTime)
    {
        firstTime = FALSE;
        yyin = source;
        yyout = listing;
    }

    currentToken = yylex();
    strncpy(tokenString, yytext, MAXTOKENLEN);

    if (lineno > prev_lineno)
    {
        // Read lines from redundant_source until redundant_lineno == lineno
        char line_buf[256]; 
        while (redundant_lineno < lineno)
        {
            if (fgets(line_buf, sizeof(line_buf), redundant_source))
            {
                redundant_lineno++;
                // Remove any newline at end of line_buf
                size_t len = strlen(line_buf);
                if (len > 0 && line_buf[len - 1] == '\n')
                {
                    line_buf[len - 1] = '\0';
                }
                // Print the source code line followed by a newline
                pc("%d: %s\n", redundant_lineno, line_buf);
            }
            else
            {
                break;
            }
        }
        prev_lineno = lineno;
    }

    if (TraceScan)
    {
        pc("\t%d: ", lineno);
        printToken(currentToken, tokenString);
    }

    return currentToken;
}

- Baixar o código
- Rodar killbuild na raiz (pra criar pastas necessárias)
- Compilar e rodar: Entrar na build e rodar: sudo bash ../VcompLabAluno.bash
- Gerar comparativo: dentro de build rodar: sudo make lexdiff
- rodar na raiz pra comparar os diffs: batcat alunodetail/*lex*diff

# Decifrando o Lab 2 de Compiladores

Partindo do Lab 1 funcional e assumindo um ambiente Linux ...

#### Copie os arquivos `parser.h`, `parser.c` e `tiny.y` (renomeie para cminus.y) da pasta  `./TinyGeracaoCodigo/` para a pasta `./src/` do seu projeto.

#### A função utilitária `void printLine(FILE *redundant_source)` do arquivo `util.c` não tem o seu protótipo declarado no arquivo `util.h`. Adicione o protótipo no arquivo `util.h` para que possa ser chamada em outros arquivos com as importações existentes.

#### `compLabAluno.bash`:
- Modifique o script para que ele compile o arquivo de saída do parser:
    ```bash
    cmake .. -DDOPARSE=TRUE
    make 
    ```


#### `main.h`:
- Desabilite o `NO_PARSER`: `#define NO_PARSE FALSE;`
- Opcionamente, desabilite a geração de código: `#define NO_CODE TRUE`
- Habilite os logs da análise sintática: `int TraceParse = TRUE;`
- Configure mode de impressão de arquivos. Substituia `LER` pela opção que for mais conveniente (opções em `lib/log.h`). Exemplo:
    ```c
    initializePrinter(detailpath, pgm, LOGALL);
    ```
- Imediatamente após a análise léxica, faça a troca de contexto de impressão em logs com a chamada à função:
    ```c
    doneLEXstartSYN(); 
    ```


#### `globals.h`: 
- A definição dos tokens parte do arquivo `parser.h` gerado pelo bison, logo substitua`typedef enum {...} TokenType;` por `typedef int TokenType;` e faça a inclusão do parser gerado bem como do token `ENDFILE` que é gerado com outro nome pelo bison (`YYEOF`):

    ```c
    #ifndef YYPARSER
    #include "parser.h"
    #define ENDFILE 0
    #endif
    ```
    Obs.: fazer essa inclusão no topo do arquivo pode ocasionar o erro de tentar fazer a importação antes do bison ter salvado o parser.c no diretório. É normal o editor/IDE entender o include do parser como erro, pois esse arquivo ainda não existe.

#### `cminus.y`:
- Você pode fazer o uso de `#define YYSTYPE ...` para definir o tipo genérico dos simbolos mas existem estratégias mais interessantes, tal como fazer uma atribuição de tipos personalizada de acordo com o token:
    ```
    %union {
        int val;
        char *name;
        TokenType token;
        TreeNode* node;
        ExpType type;
        ...
    }
    ```
    Obs.: A intuição aqui é que cada simbolo poderá ter um dos tipos declarados em `%union`.

- #### Defina o tipo de cada simbolo terminal (token):
    ```
    %token <name> ID ...
    %token <val> NUM
    %token ELSE IF INT ... PLUS MINUS ...
    ```
    Obs.: Note que os tipos são os nomes dentro de `%union`.

- #### Defina o tipo de cada simbolo não terminal:
    ```
    %type <node> programa declaracao_lista ...
    %type <type> tipo_especificador
    %type <token> soma mult relacional
    ```

- #### Raciocine com o uso das flags: `%left`, `%right`, `%start`, `%locations` dentre outras.

- #### Para contruir a árvore sintática, é preciso definir a estrutura de dados que representará o nó da árvore. Esta definição está no arquivo `globals.h`.

    ```c
    typedef struct treeNode {
        struct treeNode * child[MAXCHILDREN];
        struct treeNode * sibling;
        int lineno;
        NodeKind nodekind;
        union { StmtKind stmt; ExpKind exp;} kind;

        union {
            TokenType op;
            int val;
            char * name;
        } attr;
        
        ExpType type; /* for type checking of exps */
    } TreeNode;
    ```
    Obs.: Note que esse é um ponto de partida fornecido pelo professor. Explore da melhor forma possível para atender as necessidades da linguagem `C-`, que é significativamente mais elaborada.

- #### Note que o nó da árvore apresenta dois tipos principais de nó: `Statements` e `Expressions`. Cada um desses tipos possui subtipos, que são definidos em `globals.h`. Tome um tempo especificando os tipos de nó que você acha que são necessários para a linguagem `C-`, pois nas próximas etapas (criação da tabela de simbolos, tratamento de escopo de variáveis e de erros semânticos), cada comportamento da linguagem será associado a um tipo e subtipo de nó específico.

    ```c
    typedef enum {StmtK, ExpK} NodeKind;
    typedef enum
    {
        VarK,
        FuncK,
        AssignK,
        ReturnK,
        IfK,
        WhileK,
        ParamK
    } StmtKind;

    typedef enum
    {
        ConstK,
        IdK,
        CallK,
        OpK,
    } ExpKind;
    ```
    Obs.: Tal como o nó da arvore, adapte para atender as necessidades da linguagem `C-`.

- #### Defina os tipos que suas variáveis, funções e expressões poderão assumir no arquivo `globals.h`. Fazer as atribuições corretas nas expressões tornarão a análise semântica no Lab 3 mais fácil.

    ```c
    typedef enum {...} ExpType;
    ```

#### `util.c`:
- O arquivo `util.h` possui funções que facilitam a criação de nós da árvore sintática em função do tipo e subtipo do nó. Exemplo:

    ```c
    TreeNode * newStmtNode(StmtKind kind);
    TreeNode * newExpNode(ExpKind kind);
    ```

- Faça as modificações necessárias em `void printTree(TreeNode *tree)`, ajustando-a ao tipos e subtipos de nó criados, para que a árvore seja impressa de forma esperada.
    
- #### A escolha apropriada de tipos com o uso integrado com flex/bison vai te permitir manipular os dados de modo mais fácil. Exemplo:

    - `cminus.l`: Note que aqui você pode especificar como os dados do analizador léxico serão passados para o analizador sintático de forma explicita, fazendo o uso da `struct` `yylval`:

        ```
        {number}                {/*yylval.val = atoi(yytext)*/; return NUM;}
        {identifier}            {/*yylval.name = copyString(yytext)*/; return ID;}
        ```

    - `cminus.y`: Exemplo de como manipular os dados passados pelo analizador léxico ao definir a gramática:

        ```
        soma_expressao          : soma_expressao soma termo
                                {
                                    $$ = newExpNode(OpK);
                                    $$->attr.op = $2;
                                    $$->lineno = lineno;
                                    $$->child[0] = $1;
                                    $$->child[1] = $3;
                                }

                                | termo { $$ = $1; }
                                ;

        soma                    : PLUS { $$ = PLUS; }
                                | MINUS { $$ = MINUS; }
                                ;
        ```

- #### Declaração da Gramática
    - Com essas correções, ajustes e considerações, agora você tem um ambiente digno para começar o seu Laboratório 2, que em suma, se resume a declarar as regras de produção segundo a sintaxe do `bison` no arquivo `cminus.y`. A gramática da linguagem `C-` é fornecida no enunciado do laboratório.

# Executando o seu código
- [Opcional] Eventualmente você precisará tornar os scripts executáveis. Para isso, execute:
    ```bash
    chmod +x compLabAluno.bash
    chmod +x killbuild
    chmod +x ./scripts/* 
    ```
- Estando com o terminal na raiz do projeto:
    ```bash
    ./killbuild
    cd build
    ../compLabAluno.bash
    ```

- Para gerar arquivos com as diferenças (`.diff`):
    ```bash
    make ddiff
    ```

- Para refazer a compilação:
    ```bash
    cd ..
    ```
    ... e repita desde o início.

# Erros comuns
- Os textos estão sendo definidos como `"char *"` (e não `"string"`), logo, para copiar strings de uma variável para outra use `char *copyString(char *s)` do arquivo `util.c`. para comparar (sobretudo no Lab 3), use `strcmp(char *, char *)`.

- Ao criar um nó, o código permite que sejam passados subtipos a função criadora de um tipo que não é associado aquele subtipo, uma vez que a função não faz a verificação do subtipo passado. Exemplo: `newStmtNode(ExpK)`. Ponto a atentar: saíba a qual tipo e subtipo cada nó pertence, pois você não recebe warnings e o código compilará "normalmente" sem denunciar o erro.
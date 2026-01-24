%{
#include <stdio.h>
#include "./ast/ast.h"
#define YYDEBUG 1
extern int yylex();
int yyerror(char *);
vector<astNode*> *slist;
%}
%token VARIABLE NUMBER
%token TYPE FNAME
%token EXTERN
%left '+' '-'
%left '*' '/'
%nonassoc UMINUS
%%
declaration: '#' EXTERN TYPE FNAME '\n' functiondef
           | functiondef
           ;
functiondef: TYPE FNAME '(' ')' statement_list 
           | statement
           ;
statement: VARIABLE '=' expression
         | expression           { printf("= %d\n", $1); }
         | '{' statement_list '}'   // block statements
         ;
statement_list: statement '\n'
              | statement_list statement '\n'
              ;
expression: expression '+' NUMBER   { $$ = $1 + $3; }
          | expression '-' NUMBER   { $$ = $1 - $3; }
          | expression '*' NUMBER   { $$ = $1 * $3; }
          | expression '/' NUMBER   { $$ = $1 / $3; }
          | '-' NUMBER %prec UMINUS { $$ = -$2; } // unary operator
          | NUMBER              { $$ = $1; }
          | '(' expression ')'      { $$ = $2; }
          ;
        
%%
int yyerror(char *s) {
    fprintf(stderr, "%s\n", s);
    return 0;
}
int main(int argc, char *argv[]) {
/*
    if(argc == 2) {
        yyin = fopen(argv[1], "r");
        if (yyin == NULL) {
            fprintf(stderr, "File open error\n");
            return 1;
        }
    }
    */
    yydebug = 0;
    slist = new vector<astNode*> ();
    yyparse(); // will se the root
    // semantic analysis
    // convert to LLVM IR
    // code optimizer
    // Assembly code generator
}

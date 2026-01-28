%{
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "../ast/ast.h"
#include "semantic.h"
#define YYDEBUG 1
extern int yylex();
extern FILE *yyin;
int yyerror(const char *);
astNode *root = NULL;
%}
%union {
    int iValue;
    char *sIndex;
    astNode *nPtr;
    vector<astNode*> *stmtList;
};
%token <sIndex> VARIABLE FNAME 
%token <iValue> NUMBER
%type <nPtr> expression statement functiondef block decl extern program
%type <stmtList> statement_list decl_list
%token TYPE EXTERN IF ELSE WHILE RETURN 
%nonassoc IFX
%nonassoc ELSE

%left GE LE EQ NE '>' '<'
%left '+' '-'
%left '*' '/'
%nonassoc UMINUS
%%
program: extern extern functiondef                  {
                                                        root = createProg($1, $2, $3);
                                                    }
        | functiondef                               {
                                                        root = createProg(NULL, NULL, $1); 
                                                    }
extern: EXTERN TYPE VARIABLE '(' ')' ';'            {
                                                        $$ = createExtern($3);
                                                    }
    | EXTERN TYPE VARIABLE '(' TYPE ')' ';'           {
                                                        $$ = createExtern($3);
                                                    }
    ;
functiondef: TYPE VARIABLE '(' ')' block          {
                                                        $$ = createFunc($2, NULL, $5);
                                                    }
    | TYPE VARIABLE '(' TYPE VARIABLE ')' block        {
                                                        $$ = createFunc($2, createDecl($5), $7);
                                                    }
           ;
block: '{' decl_list statement_list '}'           {
                                                        $2->insert($2->end(), $3->begin(), $3->end());
                                                        $$ = createBlock($2);
                                                    }
     | '{' decl_list '}'                            {
                                                        $$ = createBlock($2);
                                                    }
     | '{' statement_list '}'                       {
                                                        $$ = createBlock($2);
                                                    }
     | '{' '}'                                      {
                                                        $$ = createBlock(new vector<astNode*>());
                                                    }
     ;
decl_list: decl                                     {
                                                        $$ = new vector<astNode*>();
                                                        $$->push_back($1);
                                                        }
        | decl_list decl                            {
                                                        $1->push_back($2);
                                                        $$ = $1;
                                                    }
        ;
decl: TYPE VARIABLE ';'                              {
                                                        $$ = createDecl($2);
                                                    }
    ;

statement: VARIABLE '=' expression ';'              {    
                                                        $$ = createAsgn(createVar($1), $3);
                                                    }

         | expression ';'                           {   
                                                        $$ = $1; 
                                                    }
        | block                                     {
                                                        $$ = $1;
                                                    }
        | IF '(' expression ')' statement %prec IFX     {
                                                                $$ = createIf($3, $5, NULL);
                                                    }
        | IF '(' expression ')' statement ELSE statement    {
                                                        $$ = createIf($3, $5, $7);
                                                    }
        | WHILE '(' expression ')' block            {
                                                        $$ = createWhile($3, $5);
                                                    }
        | RETURN expression ';'                     {
                                                        $$ = createRet($2);
                                                    }
         ;

statement_list: statement                           {   
                                                        $$ = new vector<astNode*> (); 
                                                        $$->push_back($1); 
                                                    }
            | statement_list statement                 {
                                                        $1->push_back($2);
                                                        $$ = $1;
                                                    }
            ;

expression: expression '+' expression   { $$ = createBExpr($1, $3, add); }
          | expression '-' expression   { $$ = createBExpr($1, $3, sub); }
          | expression '*' expression   { $$ = createBExpr($1, $3, mul); }
          | expression '/' expression   { $$ = createBExpr($1, $3, divide); }
          | expression '<' expression   { $$ = createRExpr($1, $3, lt);}
          | expression '>' expression   { $$ = createRExpr($1, $3, gt);}
          | expression GE expression   { $$ = createRExpr($1, $3, ge);}
          | expression LE expression    {$$ = createRExpr($1, $3, le);}
          | expression NE expression    { $$ = createRExpr($1, $3, neq);}
          | expression EQ expression    { $$ = createRExpr($1, $3, eq);}
          | '(' expression ')'          { $$ = $2; }
          | '-' NUMBER %prec UMINUS     { $$ = createCnst(-$2); }
          | NUMBER              {$$ = createCnst($1);}
          | VARIABLE '(' ')'            { $$ = createCall($1, NULL); }
          | VARIABLE '('expression ')'  { $$ = createCall($1, $3); }
          | VARIABLE                    { $$ = createVar($1); }
          ;
        
%%
int yyerror(const char *s) {
    fprintf(stderr, "%s\n", s);
    return 0;
}
int main(int argc, char *argv[]) {

    yydebug = 0;
    if(argc == 2) {
        yyin = fopen(argv[1], "r");
        if (yyin == NULL) {
            fprintf(stderr, "File open error\n");
            return 1;
        }
    }
    

    yyparse(); 
    //printNode(root);

    // semantic analysis
    if (root == NULL) {
        printf("Error: root is NULL\n");
        return -1;
    }
    semantic_analysis(root);
    free(root);
    // convert to LLVM IR
    // code optimizer
    // Assembly code generator
}

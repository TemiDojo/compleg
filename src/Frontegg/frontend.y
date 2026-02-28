%{
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "../ast/ast.h"
#include "semantic.h"
#define YYDEBUG 1
extern int yylex(astNode **root);
extern FILE *yyin;
int yyerror(astNode **root, const char *);
%}
%union {
    int iValue;
    char *sIndex;
    astNode *nPtr;
    vector<astNode*> *stmtList;
};
%parse-param { astNode **root }
%lex-param { astNode **root }
%token <sIndex> VARIABLE FNAME READ PRINT
%token <iValue> NUMBER
%type <nPtr> expression statement functiondef block decl extern program
%type <stmtList> statement_list decl_list
%token TYPE EXTERN IF ELSE WHILE RETURN VOID
%nonassoc IFX
%nonassoc ELSE

%left GE LE EQ NE '>' '<'
%left '+' '-'
%left '*' '/'
%nonassoc UMINUS
%%
program: extern extern functiondef                  {
                                                        *root = createProg($1, $2, $3);
                                                    }
        | functiondef                               {
                                                        *root = createProg(NULL, NULL, $1); 
                                                    }
extern: EXTERN TYPE READ '(' ')' ';'                {
                                                        $$ = createExtern($3);
                                                    }
    | EXTERN VOID PRINT '(' TYPE ')' ';'           {
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
          | PRINT '('expression')'';'  { $$ = createCall($1, $3); }

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
          | '-' expression %prec UMINUS     { $$ = createUExpr($2, uminus); }
          | NUMBER              {$$ = createCnst($1);}
          | READ '(' ')'            { $$ = createCall($1, NULL); }
          | VARIABLE                    { $$ = createVar($1); }
          ;
        
%%
int yyerror(astNode **root, const char *s) {
    fprintf(stderr, "%s\n", s);
    return 0;
}

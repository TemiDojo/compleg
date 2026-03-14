%{
#include <stdio.h>
#include <stdlib.h>
#include "../ast/ast.h"
#include "semantic.h"
#include "y.tab.h"
#define YY_DECL int yylex(astNode **root)
int yyerror(astNode **root, const char *s);
%}
%%
"int"       { return TYPE;}
"read"      { yylval.sIndex = strdup(yytext); return READ; }
"print"     { yylval.sIndex = strdup(yytext); return PRINT; }
"void"      { return VOID; }
"extern"    { return EXTERN; }
"if"        { return IF; }
"else"      { return ELSE; }
"while"     { return WHILE; }
"return"    { return RETURN; }
">="        { return GE; }
"<="        { return LE; }
"=="        { return EQ; }
"!="        { return NE; }
[0-9]+    {
        yylval.iValue = atoi(yytext);
        return NUMBER;
    }
[a-zA-Z][a-zA-Z0-9]*   {
        yylval.sIndex = strdup(yytext);
        return VARIABLE;
    }
[-()<>=+*/;{}.] {
        return *yytext;
    }

[ \t\n] ; /* ignore whitespace */
.   return yyerror(root, "Unknown character");
%%

int yywrap(void) {
    return 1;
}


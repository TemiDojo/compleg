%{
#include <stdio.h>
#include <stdlib.h>
#include "../ast/ast.h"
#include "y.tab.h"
int yyerror(const char *s);
%}
%%
"int"       { return TYPE;}
"void"      { return TYPE; }
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
[a-zA-Z]*   {
        yylval.sIndex = strdup(yytext);
        return VARIABLE;
    }
[-()<>=+*/;{}.] {
        return *yytext;
    }

[ \t\n] ; /* ignore whitespace */
.   return yyerror("Unknown character");
%%

int yywrap(void) {
    return 1;
}


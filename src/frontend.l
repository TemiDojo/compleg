%{
#include <stdio.h>
#include "y.tab.h"
extern int yylval;
%}
%%

[0-9]+    {
        yylval = atoi(yytext);
        return NUMBER;
    }
[ \t] ; /* ignore whitespace */
\n  return 0;
.   return yytext[0];
%%

int yywrap(void) {
    return 1;
}

/*
injt main(void) {
    yylex();
    return 0;
}
*/

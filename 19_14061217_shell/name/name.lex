%{
#include "name.tab.h"
#include <stdio.h>
#include <string.h>
%}
char [A-Za-z]
num [0-9]
eq [=]
sex [,.]
name {char}+
age {num}+
%%
{name} { yylval = strdup(yytext);return NAME; }
{eq} { return EQ; }
{age} { yylval = strdup(yytext);return AGE; }
{sex} { yylval = strdup(yytext);return SEX; }
%%
int yywrap() {
return 1;
}

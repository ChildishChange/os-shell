%{
#include "bison.h"
#include "global.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
%}

char[^ \t\n\>\<&]
order {char}+
space [ \t]

%%
{order}  {strcat (inputBuff,yytext); return STRING;}
\n {return END;}
\<  {strcat (inputBuff,yytext); return '<';}
\>  {strcat (inputBuff,yytext); return '>';}
&   {strcat (inputBuff,yytext); return '&';}
{space} {strcat (inputBuff,yytext);}


%%

int yywrap(){
return 1;
}

%{
#include "bison.h"
#include "global.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
%}



%%

"\n" {return END;}
"<"  {strcat (inputBuff,yytext); return '<';}
">" {strcat (inputBuff,yytext); return '>';}
"&" {strcat (inputBuff,yytext); return '&';}
{space} {strcat (inputBuff,yytext);}


%%

int yywrap(){
return 1;
}

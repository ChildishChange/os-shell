%{
#include "bison.tab.h"
#include <stdio.h>
#include <string.h>
#include "global.h"
 #define YYSTYPE char*
%}

CHAR [^" "\t\n"<"">""<""|""&"]
NUM [0-9]
SYMBOL [">""<""|""]
SPACETAB [" "\t]
EOF [-1]

OP  {SPACETAB}*{CHAR}+{SPACETAB}*
  
XX ({OP}{SYMBOL}?{OP}?)+
STRING   {XX}[&]?


%% 
{STRING}  {strcat(inputBuff,yytext);return STRING;}

{SPACETAB} 
\n       return END;


%%
int yywrap(){
return 1;
}

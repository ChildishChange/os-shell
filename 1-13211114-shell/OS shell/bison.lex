%{
	#include "bison.tab.h"
	#include "global.h"
	#include <stdio.h>
	#include <string.h>
%}

char [^" "&\<\>\n]
op [\<\>&]
blank [" "\t]
end [\n]

string ({blank}*{char}*{op}*)+

%%
{string}  {strcat(inputBuff,yytext);return STRING;}
{end} {return END;}
%%
int yywrap(){
	return 1;
}

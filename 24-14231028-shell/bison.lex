%{
#include "bison.tab.h"
#include <stdio.h>
#include <string.h>
#include "global.h"
int offset, len;
%}
character [A-Za-z/.0-9\/<>%]
string {character}*

%%
[A-Za-z0-9'%''/'' ''.''>''<''\''*''\t''|''&'/-]* {
	return STRING;
}
(\|)+ {return PIPE;}
%%

int yywrap() {
	return 1;
}

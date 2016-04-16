%{
	#include <stdio.h>
	#include "cal.tab.h"
	void yyerror(char *msg);
%}

%%
[0-9]+	{yylval = atoi(yytext);return INT;}
[-+/*\n]	return yytext[0];
[\t]	;
.	yyerror("invalid character\n");
%%

int yywrap(void)
{
	return 0;
}


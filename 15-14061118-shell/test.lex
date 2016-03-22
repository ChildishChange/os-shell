%{

int String;
int offset,flag;
char c;
#include "bison.tab.c"


 
%}
 

 
spare [' '\t]
special ['<''>''&''|'\t\0]
char[A-Za-z0-9]
order {char}+
request ({order}{special})

%%


{spare} { 
}

{request} {yylval = strdup(yytext); return STRING;
}


%%
 
int yywrap(){
	return 1;
}

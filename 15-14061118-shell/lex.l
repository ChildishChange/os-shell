%{

#include "global.h"
int offset;int flag=1;
char c;
#include "bison.h"
int i;
#include <string.h>
 
 
%}
 

 
spare [' '\t]
kongge {spare}*
 
char[A-Za-z0-9\.\/]
order {char}+


%%
[/0]  {printf("END\n");return END;}
 
{spare}  {if (flag==1) return STRING;}
{char}	{  flag=1;}
{order} { printf("zhi ling");strcpy(inputBuff,yytext); flag=0; return STRING;
}

['<'] { printf("xiaoyu");return '<';}
['>'] { printf("dayu"); return '>';}
['&'] { printf("and"); return '&';}
['|'] { printf("or"); return '|';} 


%%
 
int yywrap(){
	return 1;
}

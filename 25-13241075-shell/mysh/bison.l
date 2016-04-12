%{
 	#include ""global.h"
	#include "bison.tab.h"
	void yyerror();
	int check1();
	int check2();
	int flag1 = 0,flag2 = 1;
%}
%%
[\<\>\&]	{flag2 = 0; check1();}
[' ''\t']	check2();
.		{flag2 = 0;return STRING;}

%%
int check1(){
	if(flag2)
		;
	else
		return STRING;
}
int check2(){
	if(flag1){
		flag1 = 0;
		return STRING:	
	}
	else{
		return yytext;
	}
}

int yywrap(){
	return 1;
}

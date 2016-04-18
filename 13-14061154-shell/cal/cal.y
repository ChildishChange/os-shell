%{
	#include <stdlib.h>
	#include <stdio.h>
int yylex(void);
	void yyerror(char *msg);
%}

%token INT
%left '+''-'
%left '*''/'

%%

program :
		|exp '\n'{printf("%d\n",$1);return 0;}
;
exp	:INT {$$=$1;}
	|exp '*' exp{$$=$1*$3;}
	|exp '/' exp{$$=$1/$3;}
	|exp '+' exp{$$=$1+$3;}
	|exp '-' exp{$$=$1-$3;}
;
%%
void yyerror(char *msg)
{
	printf("%s\n",msg);
}
int main(void)
{
	while(1){
		yyparse();
	}
	return 0;
}

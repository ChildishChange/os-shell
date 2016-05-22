%{
	# include <string.h>
	# include <stdio.h>
	# include "global.h"
	int yylex();
	int yyerror();
	
	int lineend;	
	int Cmdno;
	int inredir;
	int outredir;
	int Argno;
	Cmd *(CmdList[MAX_CMD]);//不超过10条指令，主要是考虑多条管道的情况

	void addCmd(int isBack);
	void clearArgList();
	void addHistory();
	void freeCmdList();
	void init();
	void execute();
%}
%token STRING ENDLN PIPE RE_I RE_O BACK
%%
OverallCommand: 	| Command
					{
						if (lineend==1){
							execute();
							addHistory();
							freeCmdList();
						}
						
						return 0;//无论如何都要先return结束本次匹配
					}
;
Command:			SingleCommand ENDLN
					{
						addCmd(IS_FG);
						clearArgList();
						lineend = 1;
					}
					| SingleCommand BACK ENDLN
					{
						addCmd(IS_BG);
						clearArgList();
						lineend = 1;
					}
					| SingleCommand PIPE
					{
						addCmd(IS_FG);
						clearArgList();
					}
;
SingleCommand:		ArgumentCommand 
					| ArgumentCommand MultiRedirect
;
ArgumentCommand:	STRING | ArgumentCommand STRING
;
MultiRedirect: 		Redirect 
					| MultiRedirect Redirect
;
Redirect: 			Rechar STRING
;
Rechar: 			RE_I {inredir = Argno;}
					| RE_O {outredir = Argno;}
;
%%
int main() {
	init();
	printf("mysh>");
	lineend = 0;
	while(1){
		while(lineend==0) yyparse();
		lineend = 0;
		printf("mysh>");
	}
	return 0;
	
}
int yyerror() {
	printf("Syntax Error.\n");
	freeCmdList();
	clearArgList();
	lineend = 1;
}

%{
	#include "global.h"
	#include "funcvar.h"
	extern int yylex();
	extern void execute();
	int pipedes[2] = {0};
	int yyerror(char* msg);
	void debug();
%}

%token CmdArg I_ReDir O_ReDir Pipe Back EndLn

%%
ExecutableCmd	: 
		| COMMAND
		{
			execute();
			return 0;
		}
;

COMMAND		: command Pipe 
		| command EndLn
;

command		:SimpleCommand InRedirection OutRedirection back
;

SimpleCommand	:CmdArg 
		| SimpleCommand CmdArg
;

back		: 
		| Back
;

InRedirection	: 
		| I_ReDir CmdArg
;

OutRedirection	: 
		| O_ReDir CmdArg
;

%%
int yyerror(char* msg)
{
	printf("命令无效\n");
}

int main()
{
	char HostName[30];
	struct passwd *passwd;
	passwd = getpwuid(getuid());
	gethostname(HostName,30);
	init();
	while(1)
	{
		if(cmd->isPiped==0)
			printf("\033[0;36m%s@%s\033[0m:\033[1;32m%s\033[0m> ",passwd->pw_name,HostName,get_current_dir_name());
		cmd = (Command*)malloc(sizeof(Command));
		memset(cmd,'\0',sizeof(Command));
		yyparse();
	}
	return 0;
}


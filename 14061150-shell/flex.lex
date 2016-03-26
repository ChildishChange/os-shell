%{
	#include "global.h"
	#include "bison.tab.h"
	void ArgHandler();
	extern int flag_piped;
%}
CHAR [^" "\t\n|<>&]
%%
\".*\" {yytext[--yyleng]=0;strcpy(yytext,yytext+1);yyleng--;ArgHandler();return CmdArg;}
[<] {cmd->InReDir = 1;return I_ReDir;}
[>] {cmd->OutReDir = 1;return O_ReDir;}
[|] {cmd->isPiped = 1;return Pipe;}
[&] {cmd->isBack = 1;return Back;}
\n {return EndLn;}
{CHAR}+ {ArgHandler();return CmdArg;}
%%
void ArgHandler()
{
	if(cmd->InReDir == 1&&cmd->input == 0)
	{
		cmd->input = (char*)malloc(sizeof(char)*(yyleng+1));
		strcpy(cmd->input,yytext);
	}
	else if(cmd->OutReDir == 1&&cmd->output == 0)
	{
		cmd->output = (char*)malloc(sizeof(char)*(yyleng+1));
		strcpy(cmd->output,yytext);
	}
	else
	{
		strcpy(cmd->args[(cmd->argc)++],yytext);
	}
}
int yywrap()
{
	return 1;
}

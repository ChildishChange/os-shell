%{
	# include "mysh.tab.h"
	# include "global.h"
	# include <stdlib.h>
	int Argno = 0;
	char *(ArgList[MAX_ARG]);
	void addArg(char *arg);
%}
char [^" "<>\t&\|\n]
str {char}+
%%
[" "\t]*\n		{return ENDLN;}
[" "\t]*\|		{return PIPE;}
[" "\t]*{str}	{addArg(yytext);return STRING;}
[" "\t]*[&]		{return BACK;}
[" "\t]*[>] 	{return RE_O;}
[" "\t]*[<] 	{return RE_I;}
%%
int yywrap() {
	return 1;
}
void addArg(char *arg) {
	int i,j,len;
	len = strlen(arg);
	i=0;
	while (i<=len && (arg[i]==' ' | arg[i]=='\t')) i++;
	if (i!=0) for (j=0;i<=len;j++,i++) arg[j] = arg[i];
	ArgList[Argno] = (char*)malloc(sizeof(char)*strlen(arg));
	strcpy(ArgList[Argno],arg);
	Argno++;
}

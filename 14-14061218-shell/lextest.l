%{
#include "global.h"
#include "y.tab.h"
int yywrap(void);
int yylex(void);
int reInput(char *buf, int maxlen);
    #undef YY_INPUT
    #define YY_INPUT(buf, retval, maxlen)	(retval = reInput(buf, maxlen))

%}
character           ([-a-zA-Z_0-9./%])

blank_chars         ([ \f\r\t\v]+)

blank               ({blank_chars}*)

arg                 ({character}*)

args                ({blank}|(({arg}{blank_chars}{arg})*)|{arg}*)

inputRedirect       ({blank}|(<{blank}{arg}))
outputRedirect      ({blank}|(>{blank}{arg}))

progInvocation      ({character}+{args})
simpleCmd           ({progInvocation}{inputRedirect}{outputRedirect})
pipeCmd     	    ({blank}([|]{blank}{simpleCmd})+)
%%

{blank} ;
{args}|"&"       {strcat(inputBuff,yytext);return STRING;}
{inputRedirect}  {strcat(inputBuff,yytext);return inputRedirect;}
{outputRedirect} {strcat(inputBuff,yytext);return outputRedirect;}
{pipeCmd}	 {strcat(inputBuff,yytext);return pipeCmd;}
\n     return 0;


%%


int yywrap()
{
return 1;
}

int reInput(char *s, int max_len){
    int n=0;
    while((s[n++]=getc(yyin))!='\n' && n<max_len){
        if (s[n-1]<0) n--;
    }

    s[n] = 0;
    return n;
}



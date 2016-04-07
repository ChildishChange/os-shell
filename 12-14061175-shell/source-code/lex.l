%{
	#include "bison.tab.h"
	#include "global.h"
        int yywrap(void);

	int my_getinput(char *buf,int maxlen);
	#undef YY_INPUT
	#define YY_INPUT(buf,retval,maxlen)	(retval=my_getinput(buf,maxlen))
%}

%%
"<"    {strcpy(inputBuff+len,yytext); len+=strlen(yytext); return '<';}
">"    {strcpy(inputBuff+len,yytext); len+=strlen(yytext); return '>';}
"&"    {strcpy(inputBuff+len,yytext); len+=strlen(yytext); return '&';}
"|"    {strcpy(inputBuff+len,yytext); len+=strlen(yytext); return '|';}
"\n"   {inputBuff[len]=0; return 0;}
[^<>&\n \t|]+ {strcpy(inputBuff+len,yytext); len+=strlen(yytext); return STRING;}
.      {strcpy(inputBuff+len,yytext); len+=strlen(yytext);}
%%

int yywrap(){
    return 1;
}
int my_getinput(char *s,int max_len){
    int n=0;
    while ((s[n++]=getc(yyin))!='\n'&&n<max_len){
          if (s[n-1]<0) n--;
    }
    
    s[n]=0;
    return n;
}

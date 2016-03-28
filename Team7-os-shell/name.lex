%{
#include "bison.tab.h"
#include "global.h"
#include <stdio.h>
#include <string.h>
//extern char* yylval;
////((\.\/)?{char}+{none}*)|((\.\/)?{char}+{none}*[><]?{none}*{char}+)|({char}+{none}*(\/{char}*)*) echo    (([e][c][h][o])[.]*)
%}
char    [.A-Za-z0-9\.\/]
int     [0-9]
inputre [<]
outputre [>]
nothing [' ']
none    {nothing}*
commend {char}*{none}*
args    (%{int}+{none}*)|(-([a-zA-Z0-9])*{none}*)|(\|{none}*)|([<>]{none}*{char}*)|commend
enter   ({none}*[^.\|])
pipe    (\|{none}*)
%%
{commend} { strcat(inputBuff,yytext);
            
            return STRING; }
{args}    { strcat(inputBuff,yytext);
           
           return ARGS;    }
{enter}   { 
            return ENTER;  }
{inputre} { strcat(inputBuff,yytext);
            
            return INPUTRE; }
{outputre} { strcat(inputBuff,yytext);
            
            return OUTPUTRE; }
{pipe}    {
            strcat(inputBuff,yytext);
            return PIPE;}
%%        
int yywrap()
{
return 1;
}

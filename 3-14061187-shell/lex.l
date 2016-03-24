%{
    #include "global.h"
    #include "bison.tab.h"

	int yywrap(void);
	int yylex(void);
%}

character           ([-a-zA-Z_0-9.])

blank_chars         ([ \f\r\t\v]+)

blank               ({blank_chars}*)

arg                 ({character}+)

args                ({blank}|(({arg}{blank_chars}{arg})*))

inputRedirect       ({blank}|(<{blank}{arg}))
outputRedirect      ({blank}|(>{blank}{arg}))

progInvocation      ({character}+{args})
simpleCmd           ({progInvocation}{inputRedirect}{outputRedirect})

command             ({simpleCmd}(&?))
pipe_commmand       ({simpleCmd}({blank}([|]{blank}{simpleCmd})+))


%%
{pipe_commmand}     {
                        strcpy(inputBuff,yytext);
                        return PIPE;
                    }
{command}           {
                        strcpy(inputBuff,yytext);
                        return COMMAND;
                    }

\n                  {
                        BEGIN INITIAL;
                    }
<<EOF>>             {
                        BEGIN INITIAL;
                    }
%%

int yywrap(){
    return 1;
}

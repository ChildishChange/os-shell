%{
    #include "global.h"
    #include "bison.tab.h"

	int yywrap(void);
	int yylex(void);

    int my_getinput(char *buf, int maxlen);
    #undef YY_INPUT
    #define YY_INPUT(buf, retval, maxlen)	(retval = my_getinput(buf, maxlen))
%}

character           ([-a-zA-Z_0-9./%~])

blank_chars         ([ \f\r\t\v]+)

blank               ({blank_chars}*)

arg                 ({character}+)

args                ({blank}|({arg}({blank_chars}{arg})*))

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
<<EOF>>             {
                        return 0;
                    }
\n                  {
                        return 0; // yylex == 0
                    }
%%

int yywrap(){
    return 1;
}

int my_getinput(char *s, int max_len){
    int n=0;
    while((s[n++]=getc(yyin))!='\n' && n<max_len){
        if (s[n-1]<0) n--;
    }

    s[n] = 0;
    return n;
}


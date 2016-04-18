%{
    #include "global.h"
    #include "bison.tab.h"  
	int yywrap(void);
	int yylex(void);

int my_getinput(char *buf, int maxlen);
    #undef YY_INPUT
    #define YY_INPUT(buf, retval, maxlen)	(retval = my_getinput(buf, maxlen))    
%}


blank_chars         ([ \t\v\f\r]+)

blank               ({blank_chars}*)

character           ([-a-zA-Z_0-9./%])

arg                 ({character}+)

args                ({blank}|({arg}({blank_chars}{arg})*))

outputRedirect      ({blank}|(>{blank}{arg}))
inputRedirect       ({blank}|(<{blank}{arg}))


progInvocation      ({character}+{args})
simpleCmd           ({progInvocation}{inputRedirect}{outputRedirect})


pipe_commmand       ({simpleCmd}({blank}([|]{blank}{simpleCmd})+))

command             ({simpleCmd}(&?))

%%
{pipe_commmand}     {
                        strcpy(inputBuff,yytext);   //lex将识别出来的单

                //词存放在yytext[]字符数据中，因此该数组的内容就代表了所识别出来的单词的内容。
                        return pipe_command;
                    }
{command}           {
                        strcpy(inputBuff,yytext);
                        return command;
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


%{
    #include "global.h"

    extern int yylex ();
    void yyerror ();
    void addwork();
    int offset, len, commandDone;
%}

%token  COMMAND
%token  PIPE
%%
line    :   /* empty */
            |PIPE   {
                        execute2();
                        commandDone = 1;
                        addwork();
                    }
            |COMMAND
                    {
                        execute();
                        commandDone = 1;
                        addwork();
                    }
%%


/****************************************************************
                  错误信息执行函数
****************************************************************/
void yyerror()
{
    printf("你输入的命令不正确，请重新输入！\n");
    addwork();
}

void addwork(){
    if(commandDone == 1){ //命令已经执行完成后，添加历史记录信息
        commandDone = 0;
        addHistory(inputBuff);
    }

    tcsetpgrp(0,getpid());
}
/****************************************************************
                  main主函数
****************************************************************/
int main(int argc, char** argv) {
    int i;
    char c;
    extern FILE* yyin;
	yyin = stdin;
    init(); //初始化环境
    commandDone = 0;

    printf("yourname@computer:%s$ ", get_current_dir_name()); //打印提示符信息

    while(1){
        yyparse(); //调用语法分析函数，该函数由yylex()提供当前输入的单词符号
        printf("yourname@computer:%s$ ", get_current_dir_name()); //打印提示符信息
		//sleep(1);
		//printf("***");
        //while((c=getchar())!=EOF) printf("*%c*",c);
     }

    return (EXIT_SUCCESS);
}


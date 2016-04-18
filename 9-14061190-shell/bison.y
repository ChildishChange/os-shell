%{
    #include "global.h"

    extern int yylex ();
    void yyerror ();
    void add_history();
    int offset, len, commandDone;
%}

%token  command
%token  pipe_command
%%
line    :   /* empty */
            |pipe_command   {
                        execute_pipe();
                        commandDone = 1;
                        add_history();
                    }
            |command
                    {
                        execute();
                        commandDone = 1;
                        add_history();
                    }
%%


/****************************************************************
                  错误信息执行函数
****************************************************************/
void yyerror()
{
    printf("你输入的命令不正确，请重新输入！\n");
    add_history();
}

void add_history(){
    if(commandDone == 1){ //命令已经执行完成后，添加历史记录信息
        commandDone = 0;
        addHistory(inputBuff);
    }
    printf("user-sh@:%s> ", get_current_dir_name()); //打印提示符信息
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

    printf("user-sh@:%s> ", get_current_dir_name()); //打印提示符信息

    while(1){
        yyparse(); //调用语法分析函数，该函数由yylex()提供当前输入的单词符号
		//sleep(1);        
		//printf("***");
        //while((c=getchar())!=EOF) printf("*%c*",c);
     }

    return (EXIT_SUCCESS);
}


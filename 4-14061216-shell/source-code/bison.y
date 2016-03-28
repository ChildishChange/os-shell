//这只是分析程序，最终要编译成一个完整的C程序
//定义段，出现在最终产生的c文件中
%{
    #include "global.h"

   // int yylex ();
    void yyerror ();
    #define YYSTYPE char*
    int offset, len, commandDone;
%}
//声明标记
%token STRING 
%token END

//结束符
%%

line            :                             
                    |command END                   { execute();  commandDone = 1;  return 0; }
;
//命令后面加 & 代表后台运行
command         :   fgCommand
                    |fgCommand '&'
;

fgCommand       :   simpleCmd
;
// ...<.
// ....  
// ..>.
// ..<.>.    
simpleCmd       :   progInvocation inputRedirect outputRedirect
;

progInvocation  :   STRING args
;

inputRedirect   :   /* empty */
                    |'<' STRING
;

outputRedirect  :   /* empty */
                    |'>' STRING
;

args            :   /* empty */
                    |args STRING 
;

%%


/****************************************************************
                  错误信息执行函数
****************************************************************/
//OR
void yyerror()
{
    printf(" is a wrong input\n");
}

/****************************************************************
                  main主函数
****************************************************************/
int main(int argc, char** argv) {
    int i;
    char c;

    init(); //初始化环境
    commandDone = 0;
    
    printf("4_14061216_Shell:%s$ ", get_current_dir_name()); //打印提示符信息

    while(1){
        //yacc语法分析程序的入口点，调用开始启动分析。分析成功返回0，否则返回非零值
        c=getchar();
        if(c!=-1){
		inputBuff[0]=c;
	}
        yyparse(); //调用语法分析函数，该函数由yylex()提供当前输入的单词符
        if(commandDone == 1){ //命令已经执行完成后，添加历史记录信息
            commandDone = 0;
            addHistory(inputBuff);
        }
	for(i=0;i<100;i++){
		inputBuff[i]=0;
	}
        printf("4_14061216_Shell:%s$ ",get_current_dir_name()); //打印提示符信息
     }

    return (EXIT_SUCCESS);
}

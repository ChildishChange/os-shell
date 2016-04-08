%{
#include "global.h"
extern int yylex();
void yyerror ();
void addwork();
int offset, len, commandDone;
%}

%token STRING 
%token inputRedirect 
%token outputRedirect
%token pipeCmd
%%
line            :   /* empty */
                    |command                      {   execute(); strcpy(inputBuff,"");
 commandDone = 1; 
addHis();}
;

command         :   fgCommand
                    |fgCommand '&'
		
;

fgCommand       :   simpleCmd
;

simpleCmd       :   progInvocation inputRe outputRe
		    | simpleCmd PipeCmd
;
PipeCmd		:   pipeCmd
;

progInvocation  :   STRING args
;

inputRe		:   /*empty*/
		    | inputRedirect
;
outputRe	:   /*empty*/
		    | outputRedirect
;

args            :   /* empty */
                    |args STRING
;


%%
/****************************************************************
                  错误信息执行函数
****************************************************************/
void yyerror()
{
    printf("wrong input,plz try again!\n");
	addHis();
}


void addHis(){
    if(commandDone == 1){ //命令已经执行完成后，添加历史记录信息
        commandDone = 0;
        addHistory(inputBuff);
    }
    printf("1406OS:%s$ user-sh> ", get_current_dir_name()); //打印提示符信息
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

    printf("1406OS:%s$ user-sh> ", get_current_dir_name()); //打印提示符信息

    while(1){
        yyparse(); //调用语法分析函数，该函数由yylex()提供当前输入的单词符号
     }

    return (EXIT_SUCCESS);
}


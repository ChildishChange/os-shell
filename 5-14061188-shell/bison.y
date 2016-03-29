%{
    #include "global.h"
	#include <string.h>
    #define YYSTYPE char*

	void mallocTmp();
	void dumpTmp();
	void freeCmds();
	void pTmp();
	void pCmds();
	void a1();
	void a2();
	void a3();

	int commandDone, n;
	SimpleCmd *tmp;
	SimpleCmd **cmds;
%}

%token STRING

%%

line            :	'\n'	{return 0;}
					|command '\n'	{
						cmds = (SimpleCmd**)malloc(sizeof(SimpleCmd*));
						n = 0;
						cmds[n++] = tmp;
						mallocTmp();
				        executeCmds(cmds, n);		
						dumpTmp();
                        freeCmds();
						commandDone = 1;
						return 0;
					}
					|chainCmd command '\n'	{
						cmds[n++] = tmp;
						mallocTmp();
						executeCmds(cmds, n);
						dumpTmp();
						freeCmds();
						commandDone = 1;
						return 0;
					}
;

chainCmd		:	command '|'	{
						cmds = (SimpleCmd**)malloc(sizeof(SimpleCmd*));
						n = 0;
						cmds[n++] = tmp;
						mallocTmp();
					}
					|chainCmd command '|'	{
						cmds[n++] = tmp;
						mallocTmp();
					}

command         :	simpleCmd 
					|simpleCmd '&'	{
						tmp->isBack = 1;	
					}
;

simpleCmd       :   progInvocation inputRedirect outputRedirect
;

progInvocation  :   STRING args	{
						(tmp->args)[0] = strdup($1);
					}
;

inputRedirect   :   /* empty */
                    |'<' STRING 	{
						tmp->input = strdup($2);
					}
;

outputRedirect  :   /* empty */
                    |'>' STRING		{
						tmp->output = strdup($2);
					}
;

args            :   /* empty */		
                    |args STRING	{
						if (tmp->argc > ARGCMAX) {
							printf("Too many args.\n");
							dumpTmp();
							commandDone = 1;
							return 0;
						} else {
							(tmp->args)[(tmp->argc)++] =  strdup($2);
						}
					}
;

%%

/****************************************************************
                  测试用函数
****************************************************************/
void pTmp(){
	int i;
	printf("tmp status:\n");
	printf("isback:%d\nargc:%d\ninput:%s\noutput:%s\n", tmp->isBack, tmp->argc, tmp->input, tmp->output);
	for (i = 0; i <= tmp->argc - 1; i++){
		printf("args[%d]:%s\n", i, tmp->args[i]);
	}
}

void pCmds(){
	int i, j;
	for (i = 0; i <= n - 1; i++){
		printf("cmd[%d] status:\n", i);
		printf("isback:%d\nargc:%d\ninput:%s\noutput:%s\n", cmds[i]->isBack, cmds[i]->argc, cmds[i]->input, cmds[i]->output);
		for (j = 0; j <= cmds[i]->argc - 1; j++){
			printf("args[%d]:%s\n", j, cmds[i]->args[j]);
		}
	}
}

void a1(){
	printf("anchor1.\n");
}

void a2(){
	printf("anchor2.\n");
}

void a3(){
	printf("anchor3.\n");
}

/****************************************************************
                  内存分配函数
****************************************************************/
void mallocTmp(){
	tmp = (SimpleCmd*)malloc(sizeof(SimpleCmd));
	tmp->isBack = 0;
	tmp->argc = 1;
	tmp->pflag = 0;
	tmp->input = NULL;
	tmp->output = NULL;
}

void dumpTmp(){
	int i;
	for (i = 0; i <= tmp->argc - 1; i++){
		free((tmp->args)[i]);
		tmp->args[i] = NULL;
	}
	tmp->pflag = 0;
	tmp->argc = 1;
	tmp->isBack = 0;
	free(tmp->input);
	tmp->input = NULL;
	free(tmp->output);
	tmp->output = NULL;
}

void freeCmds(){
	int i, j;
	for (i = 0; i <= n - 1; i++){
		for (j = 0; j <= cmds[i]->argc - 1; j++){
			free((cmds[i]->args)[j]);
			(cmds[i]->args)[j] = NULL;
		}
		free(cmds[i]->input);
		cmds[i]->input = NULL;
		free(cmds[i]->input);
		cmds[i]->output = NULL;
		free(cmds[i]);
		cmds[i] = NULL;
	}
	free (cmds);
	cmds = NULL;
	n = 0;
}

/****************************************************************
                  错误信息执行函数
****************************************************************/
int yyerror()
{
    printf("你输入的命令不正确，请重新输入！\n");
}

/****************************************************************
                  main主函数
****************************************************************/
int main(int argc, char** argv) {
    init(); //初始化环境
    commandDone = 0;
	mallocTmp();
    printf("atea-sh:"); //打印提示符信息

    while(1){
		
        yyparse(); //调用语法分析函数，该函数由yylex()提供当前输入的单词符号
        if(commandDone == 1){ //命令已经执行完成后，添加历史记录信息
            commandDone = 0;
            addHistory(inputBuff);
        }
        
        printf("atea-sh:"); //打印提示符信息
    }
	
    return (EXIT_SUCCESS);
}

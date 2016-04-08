////change log:109lines 3/23 19:19

%{
    #include "global.h"
    int yylex();
    void yyerror ();
    int offset, len, commandDone;
%}

%token STRING
%token ARGS
%token ENTER
%token INPUTRE 
%token OUTPUTRE
%token PIPE

%%
line            :   /* empty */
                    |command                     
                    {   //printf("1"); 
                              execute();  commandDone = 1;   }
;

command         :   fgCommand                      {//printf("2");
}
;

fgCommand       :   simpleCmd                        {//printf("3");
}
;

simpleCmd       :   progInvocation inputRedirect outputRedirect   
                    |progInvocation pipe progInvocation                                          {//printf("4");
}
;

progInvocation  :   STRING
                    |STRING args               {//printf("5");
}
;

inputRedirect   :   /* empty */ 
                    |inputre STRING                     {//printf("6");
}
;

outputRedirect  :   /* empty */
                    |outputre STRING                        {//printf("7");
}
;
args            :   /* empty */
                   |args ARGS                    {//printf("8");
}
;
inputre         :  /*empty*/
                    | INPUTRE             {//printf("9");
}     
; 
outputre        :   /*empty*/
                    | OUTPUTRE           {//printf("10");
}
;
pipe            :   /*empty*/
                    | PIPE
;
      

%%

/****************************************************************
                  词法分析函数
****************************************************************/

/****************************************************************
                  错误信息执行函数
****************************************************************/
void yyerror()
{
    //printf("Your recommend is illegal,Please input again!\n");
}

/****************************************************************
                  main主函数
****************************************************************/
int main(int argc, char** argv) {
    int i;
    //char c;

    init(); //初始化环境
    commandDone = 0;
    
    printf("Team7@computer:%s$ ", get_current_dir_name()); //打印提示符信息

    while(1){
        char c;
        /*i = 0;
        while((c=getchar())==-1);
        ungetc(c,stdin);/////////////////////////change in 2016/3/23 19:19
        while((c = getchar()) != '\n'){ //读入一行命令
            inputBuff[i++] = c;
        }
        inputBuff[i] = '\0';

        len = i;
        offset = 0;*/
        while((c=getchar())==-1);
        ungetc(c,stdin);
        yyparse(); //调用语法分析函数，该函数由yylex()提供当前输入的单词符号
        
        if(commandDone == 1){ //命令已经执行完成后，添加历史记录信息
            commandDone = 0;
            addHistory(inputBuff);
            
        }
        
        printf("Team7@computer:%s$ ", get_current_dir_name()); //打印提示符信息
        memset(inputBuff, 0, sizeof(inputBuff));
     }

    return (EXIT_SUCCESS);
}


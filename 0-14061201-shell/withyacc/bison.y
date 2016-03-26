%{
    #include "global.h"
    SimpleCmd * tmpcmd;
    Argc * argcv;
    int yylex ();
    void yyerror ();
      
    int offset, len, commandDone;

%}

%token STRING

%%
line            :   '\n' {          return 0;            }
                    |command '\n'   {
                        execute();
                        commandDone = 1; 
                        return 0; 
                    }
;

command         :   fbCommand 
                    |fbpCommand  
;

fbpCommand      :   pCommand   
                    |pCommand '&'  
;

pCommand        :   pCommand '|' simpleCmd 
                    |simpleCmd '|' simpleCmd
;

fbCommand       :   simpleCmd   
                    |simpleCmd '&' 
;

simpleCmd       :   args inputRedirect outputRedirect 
;

inputRedirect   :   /* empty */    
                    |'<' STRING    
;

outputRedirect  :   /* empty */    
                    |'>' STRING    
;

args            :   STRING    
                    |args STRING   
;

%%

/****************************************************************
                  词法分析函数
****************************************************************/

//由lex.l文件提供

/****************************************************************
                  错误信息执行函数
****************************************************************/
void yyerror()
{
    printf("你输入的命令不正确，请重新输入！\n");
}

/****************************************************************
                  main主函数
****************************************************************/
int main(int argc, char** argv) {
    int i;
    char c;

    commandDone = 0;
    offset = 0;
    len = 0;
    init(); //初始化环境
    
   printf("********************************************************************\n");
   printf("*      author:zhangshaojun,yangjian                                *\n");
   printf("*      author:huhantao,chrngfurui                                  *\n");
   printf("********************************************************************\n");

    while(1){

        ctrl_cz();

        if (tmpcmd!=NULL) 
	free(tmpcmd);
        tmpcmd = NULL;
        if (argcv!=NULL) 
	free(argcv);
        argcv = NULL;
  
        yyparse(); //调用语法分析函数，该函数由yylex()提供当前输入的单词符号

        if(commandDone == 1){ //命令已经执行完成后，添加历史记录信息
            commandDone = 0;
            addHistory(inputBuff);
        }
        
     }

    return (EXIT_SUCCESS);
}

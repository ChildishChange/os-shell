%{
    #include "global.h"
    #include <string.h>
    #define YYSTYPE string
    typedef char* string;
    void yyerror ();     
    int yylex();
    int offset, len, commandDone;
    int err;
%}

%token STRING END

%%
line            :   /* empty */ END {   if(!err) return;}
                    |command  END   {   if(!err){
                                            execute();
                                            commandDone = 1;
                                            }

                                            return; }
;

command         :   fgCommand
                    |fgCommand '&'
;

fgCommand       :   simpleCmd
;

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
                  词法分析函数
****************************************************************/
// int yylex(){
//     int flag;
//     char c;
    
//     while(offset < len && (inputBuff[offset] == ' ' || inputBuff[offset] == '\t')){ 
//         offset++;
//     }
    
//     flag = 0;
//     while(offset < len){ 
//         c = inputBuff[offset];
        
//         if(c == ' ' || c == '\t'){
//             offset++;
//             return STRING;
//         } 
//         if(c == '<' || c == '>' || c == '&'){
//             if(flag == 1){
//                 flag = 0;
//                 return STRING;
//             }
//             offset++;
//             return c;
//         }
        
//         flag = 1;
//         offset++;
//     }
    
//     if(flag == 1){
//         return STRING;
//     }else{
//         return 0;
//     }
// }

/****************************************************************
                  错误信息执行函数
****************************************************************/
void yyerror()
{
    printf("你输入的命令不正确，请重新输入！\n");
    err = 1;
}

/****************************************************************
                  main主函数
****************************************************************/
int main(int argc, char** argv) {
    int i;
    char c;

    init(); //初始化环境
    commandDone = 0;
    
    printf("user-sh> "); //打印提示符信息

    while(1){
        if(!err){

        c = getchar();
        if(c != -1)
            inputBuff[0] = c;

        yyparse(); //调用语法分析函数，该函数由yylex()提供当前输入的单词符号

        if(commandDone == 1){ //命令已经执行完成后，添加历史记录信息
            commandDone = 0;
            addHistory(inputBuff);

        }
                    
        for(i = 0;i < 100; i++){
            inputBuff[i] = 0;
        }
    }
    else{
        err = 0;
    }
            
        printf("user-sh> "); //打印提示符信息
        
    }
    return (EXIT_SUCCESS);
}

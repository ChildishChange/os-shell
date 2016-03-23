%{
    #include "global.h"

    int yylex ();
    void yyerror ();
    SimpleCmd* mEcho(SimpleCmd* head);

    SimpleCmd * mcmd;
    argC * argcv;
      
    int offset, len, commandDone;

%}

%token STRING

%%
line            :   '\n' {return 0;}
                    |command '\n'   {
                        $$ = $1;
                        if ($$==NULL) return 0; 
                        //printf("line:%d\n",$$);
                        //printf("%s %s %s %s\n",mcmd->input,mcmd->output,argcv->s,argcv->next==NULL?"(null)":argcv->next->s); 
                        //strcpy(inputBuff,$1);   
                        //execute();  
                        for (mcmd = (SimpleCmd*)$$;mcmd!=NULL;mcmd = mcmd->next)
                        {
                            len = 0;
                            for (argcv = mcmd->ac;argcv!=NULL;argcv = argcv->next, ++len);
                            if (len) 
                            {
                                mcmd->argc = len;
                                mcmd->args = (char**)malloc(sizeof(char*)*len);
                                for (argcv = mcmd->ac;argcv!=NULL;argcv = argcv->next)
                                {
                                    mcmd->args[--len] = strdup(argcv->s);
                                }
                            }
                        }
                        //printf("ln:%d\n",$$);
                        mEcho((SimpleCmd*)$$);
                        execSimpleCmd((SimpleCmd*)$$);
                        commandDone = 1; 
                        return 0; 
                    }
;

command         :   fbpCommand  {
                        $$ = $1;    
                        //printf("pC:%d\n",$$);
                    }
                    |fbCommand  {
                        $$ = $1;
                        //printf("sC:%d\n",$$);
                    }
;

fbpCommand      :   pCommand   { $$ = $1;}
                    |pCommand '&'  {
                        $$ = $1;  
                        mcmd = (SimpleCmd*)$$;
                        mcmd->isBack = 1;                        
                    }
;

pCommand        :   pCommand '|' simpleCmd {
                        $$ = $1;
                        for (mcmd = (SimpleCmd*)$$;mcmd->next!=NULL;mcmd = mcmd->next);
                        mcmd->next = $3;                                
                    }
                    |simpleCmd '|' simpleCmd {
                        $$ = $1;
                        mcmd = (SimpleCmd*)$$;
                        mcmd->next = $3;                                                
                    }
;

fbCommand       :   simpleCmd   { $$ = $1;}
                    |simpleCmd '&'  {
                        $$ = $1;  
                        mcmd = (SimpleCmd*)$$;
                        mcmd->isBack = 1;                        
                    }
;

simpleCmd       :   args inputRedirect outputRedirect {
                        $$ = $1;
                        //printf("sim:%d\n",$$);
                        //strcat($$,$2);
                        //strcat($$,$3);
                        mcmd = (SimpleCmd*)$$;
                        mcmd->input = $2;
                        mcmd->output = $3;
                    }
;

inputRedirect   :   /* empty */     {
                        $$ = NULL;
                    }
                    |'<' STRING     {
                        //$$ = strdup("<");
                        //strcat($$,$2);
                        $$ = strdup($2);
                    }
;

outputRedirect  :   /* empty */     {
                        $$ = NULL;
                    }
                    |'>' STRING     {
                        //$$ = strdup(">");
                        //strcat($$,$2);
                        $$ = strdup($2);
                    }
;

args            :   STRING     {
                        mcmd = (SimpleCmd*)malloc(sizeof(SimpleCmd));
                        mcmd->isBack = 0;
                        mcmd->argc = 0;
                        mcmd->input = NULL;
                        mcmd->output = NULL;
                        mcmd->next = NULL;
                        mcmd->ac = NULL;
                        $$ = mcmd;
                        //printf("args:%d\n",$$);
                        mcmd = (SimpleCmd*)$$;
                        argcv = (argC*)malloc(sizeof(argC));
                        argcv->next = mcmd->ac;                            
                        argcv->s = strdup($1);
                        mcmd->ac = argcv;
                    }
                    |args STRING    {
                        //$$ = strdup($1);
                        //strcat($$," ");
                        //strcat($$,$2);
                        if ($1==NULL) 
                        {
                            mcmd = (SimpleCmd*)malloc(sizeof(SimpleCmd));
                            mcmd->isBack = 0;
                            mcmd->argc = 0;
                            mcmd->input = NULL;
                            mcmd->output = NULL;
                            mcmd->next = NULL;
                            mcmd->ac = NULL;
                            $$ = mcmd;
                        }
                        else $$ = $1;
                        //printf("args:%d\n",$$);
                        mcmd = (SimpleCmd*)$$;
                        argcv = (argC*)malloc(sizeof(argC));
                        argcv->next = mcmd->ac;                            
                        argcv->s = strdup($2);
                        mcmd->ac = argcv;
                    }
;

%%


SimpleCmd* mEcho(SimpleCmd* head)
{
    SimpleCmd* p = head;
    SimpleCmd* tmp;
    int i=0,j;
    for (;p!=NULL;p=p->next)
    {
        printf("%d: %d\n",++i,p->argc);
        for (j=0;j<p->argc;++j)
        {
            printf("%s ",p->args[j]);
        }
        printf("\nInput: %s\nOutput: %s\n",p->input,p->output);
        if (p->args[p->argc]!=NULL) 
        {
//            printf("overflow:\n");
//            printf("%s.\n",p->args[p->argc]);
            p->args[p->argc] = NULL;
        }
    }
    return NULL;
}

/****************************************************************
                  词法分析函数
****************************************************************/
int yylex_(){
    //这个函数用来检查inputBuff是否满足lex的定义，实际上并不进行任何操作，初期可略过不看
    int flag;
    char c;
    
	//跳过空格等无用信息
    while(offset < len && (inputBuff[offset] == ' ' || inputBuff[offset] == '\t')){ 
        offset++;
    }
    
    flag = 0;
    while(offset < len){ //循环进行词法分析，返回终结符
        c = inputBuff[offset];
        
        if(c == ' ' || c == '\t'){
            offset++;
            return STRING;
        }
        
        if(c == '<' || c == '>' || c == '&'){
            if(flag == 1){
                flag = 0;
                return STRING;
            }
            offset++;
            return c;
        }
        
        flag = 1;
        offset++;
    }
    
    if(flag == 1){
        return STRING;
    }else{
        return 0;
    }
}

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

    init(); //初始化环境
    commandDone = 0;
    
    printf("\n");
    printf("%s\n","/***********************************************************\\");
    printf("\n");
    printf("%s\n","*    Excited! Shell 1.0.0.0                                 *");
    printf("\n");
    printf("%s\n","*    Authors : Zhang Wei, Feng Weitao.                      *");
    printf("\n");
    printf("%s\n","*    Excited Studio (c) 2016-2016 Part of Rights Reserved.  *");
    printf("\n");
    printf("%s\n","\\***********************************************************/");

    while(1){

        printLine();

//        i = 0;
//        while((c = getchar()) != '\n'){ //读入一行命令
//            inputBuff[i++] = c;
//        }
//        inputBuff[i] = '\0';

//        len = i;
//        offset = 0;

        if (mcmd!=NULL) free(mcmd);
        mcmd = NULL;
        if (argcv!=NULL) free(argcv);
        argcv = NULL;
  
        yyparse(); //调用语法分析函数，该函数由yylex()提供当前输入的单词符号

        if(commandDone == 1){ //命令已经执行完成后，添加历史记录信息
            commandDone = 0;
            addHistory(inputBuff);
        }
        
     }

    return (EXIT_SUCCESS);
}

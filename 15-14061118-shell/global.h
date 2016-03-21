#ifndef _global_H
#define _global_H
#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifdef	__cplusplus
extern "C" {
#endif   
    
    #define HISTORY_LEN 10
    

    #include <stdio.h>
    #include <stdlib.h>
    
    typedef struct SimpleCmd {
        int isBack;     // 是否后台运行
        char **args;    // 命令及参数
        char *input;    // 输入重定向
        char *output;   // 输出重定向
		int pipeIn,pipeOut;
		struct SimpleCmd *next;
    } SimpleCmd;

    typedef struct History {
        int start;                    //首位置
        int end;                      //末位置
        char cmds[HISTORY_LEN][100];  //历史命令
    } History;

    
    
    char inputBuff[100];  //存放输入的命令
    
    void init();
    void addHistory(char *history);
    void execute();

#ifdef	__cplusplus
}
#endif

#endif	/* _global_H */

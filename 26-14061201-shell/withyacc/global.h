#ifndef _global_H
#define _global_H

#ifdef	__cplusplus
extern "C" {
#endif   
    
    #define HISTORY_LEN 10
    
    #define STOPPED "stopped"
    #define RUNNING "running"
    #define DONE    "done"
    #define KILLED "interrupted"
   
    #include <stdio.h>
    #include <stdlib.h>
    #include <pwd.h>
    #include <unistd.h>
    typedef struct asChain {
        char *s;
        struct asChain* next;
    } Argc;
    typedef struct SimpleCmd {
        int isBack,argc;     // 是否后台运行
        char **args;    // 命令及参数
        char *input;    // 输入重定向
        char *output;   // 输出重定向
        struct SimpleCmd* next;
         Argc *ac;
    } SimpleCmd;

    typedef struct History {
        int start;                    //首位置
        int end;                      //末位置
        char cmds[HISTORY_LEN][100];  //历史命令
    } History;

    typedef struct Job {
        int pid;          //进程号
        char cmd[100];    //命令名
        char state[10];   //作业状态
        struct Job *next; //下一节点指针
    } Job;
     typedef struct PipeNode {
        pid_t pid;
        int writePipe, readPipe, status;
        struct PipeNode* next;
    } pNode;
    char *inputBuff;  //存放输入的命令
    
    void init();
    void addHistory(char *history);
    void execute();

#ifdef	__cplusplus
}
#endif

#endif	/* _global_H */

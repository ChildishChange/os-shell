#ifndef _global_H
#define _global_H

#ifdef	__cplusplus
extern "C" {
#endif   
    
    #define HISTORY_LEN 10
    
    #define STOPPED "stopped"
    #define RUNNING "running"
    #define DONE    "done"

    #include <stdio.h>
    #include <stdlib.h>
    
    typedef struct SimpleCmd {
        int isBack;     // 是否后台运行
        char **args;    // 命令及参数
        char *input;    // 输入重定向
        char *output;   // 输出重定向
    } SimpleCmd;

    typedef struct ComplexCmd {
        int isBack;
        int num;
        SimpleCmd **cmds;
    } ComplexCmd;
 
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
    
    char inputBuff[100];  //存放输入的命令

    int len;    

    void init();
    void addHistory(char *history);
    void execute();
    
    static char* builtin[]={".",":","[","alias","bg","bind","break","builtin","caller","cd",
                                "command","compgen","complete","compopt","continue",
                                "declare","dirs","disown","echo","enable","eval","exec","exit",
                                "export","false","fc","fg","getopts","hash","help","history",
                                "jobs","kill","let","local","logout","mapfile","popd","printf",
                                "pushd","pwd","read","readarray","readonly","return","set",
                                "shift","shopt","suorce","suspend","test","times","trap","true",
                                "type","typeset","ulimit","umask","unalias","unset","wait",NULL
                            };

#ifdef	__cplusplus
}
#endif

#endif	/* _global_H */

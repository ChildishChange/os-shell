#ifndef _global_H
#define _global_H

#ifdef	__cplusplus
extern "C" {
#endif   
    
    #define HISTORY_LEN 10
	#define ARGCMAX 10
	#define ARGLENMAX 40
    #define PIPEBUFSIZE 4096
    
    #define STOPPED "stopped"
    #define RUNNING "running"

    #include <stdio.h>
    #include <stdlib.h>
    
    typedef struct SimpleCmd {
        int isBack;     // 是否后台运行
        char *input;    // 输入重定向
        char *output;   // 输出重定向
		int argc;
		int pflag;
        char *args[ARGCMAX];    // 命令及参数
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
    
    char *inputBuff;  //存放输入的命令
    
    void init();
    void addHistory(char *history);
	void executeCmds(SimpleCmd **cmds, int n);

#ifdef	__cplusplus
}
#endif

#endif	/* _global_H */

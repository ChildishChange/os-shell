#ifndef _global_h
	#define _global_h

	#include <stdio.h>
	#include <stdlib.h>
	#include <unistd.h>
	#include <string.h>
	#include <pwd.h>
	#include <fcntl.h>
	#include <ctype.h>
	#include <math.h>
	#include <errno.h>
	#include <signal.h>
	#include <stddef.h>
	#include <sys/types.h>
	#include <sys/stat.h>
	#include <sys/wait.h>
	#include <sys/ioctl.h>
	#include <sys/termios.h>
	   
	#define HISTORY_LEN 100
	
	#define STOPPED "已停止"
	#define RUNNING "运行中"
	#define DONE    "已结束"
	
	typedef struct Command {
	        int isBack;     	// 是否后台运行
		int isPiped;		// 是否是管道的输入
		int aftPipe;		// 是否是管道的输出
		int InReDir;		// 是否有输入重定向
		int OutReDir;		// 是否有输出重定向
		int argc;		// 命令和参数的总数
        	char args[30][40];	// 命令及参数
        	char *input;    	// 输入重定向
        	char *output;   	// 输出重定向
	    } Command;

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

	Command *cmd;

#endif


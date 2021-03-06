#ifndef GLOBAL_H
#define GLOBAL_H

	/*---------------------------------------×
	 |        	  常量宏/结构体定义           |
	 ×---------------------------------------*/

	#define MAX_CMD 10
	#define MAX_ARG 10
	#define IS_BG	1
	#define IS_FG	0
	#define STATE_RUN 1
	#define STATE_HANG 0
	#define MAX_HISLEN 100
	#define MAX_ENVPATH 10
	#define MAX_STRING_LENGTH 100

	typedef struct Cmd {
		int isBack;
		char **args;
		int input;
		int output;
	} Cmd;
	typedef struct Job {
		int pid;
		char cmd[MAX_STRING_LENGTH];
		char state;
		struct Job *next;
	} Job;
	typedef struct History {
		char cmd[MAX_HISLEN];
		struct History *next;
	} History;


	/*---------------------------------------×
	 |        	  全局变量/函数声明           |
	 ×---------------------------------------*/

	extern int Cmdno;
	extern int Argno;
	extern Cmd* (CmdList[MAX_CMD]);
	extern char* (ArgList[MAX_ARG]);
	extern History *HisList;
	extern int inredir;
	extern int outredir;


#endif

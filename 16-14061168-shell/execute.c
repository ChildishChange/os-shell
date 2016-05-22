# include <stdio.h>
# include <stdlib.h>
# include <string.h>
# include <unistd.h>
# include <signal.h>
# include <fcntl.h>
# include <sys/wait.h>
# include <sys/types.h>
# include <sys/stat.h>
# include "global.h"

/*-----------------------------------------×
 |           变量、函数声明部分             |
 ×-----------------------------------------*/
//输入相关声明	
int Cmdno;
int Argno;
Cmd *(CmdList[MAX_CMD]);
char* (ArgList[MAX_ARG]);
History *HistList;
int inredir;
int outredir;
void printHistory();

//运行相关声明
Job *JobList = NULL;
int goon=0;
int fgpid = 0,fggpid;
char CmdBuff[MAX_STRING_LENGTH];
char *EnvPath[MAX_ENVPATH];
void printJobs();
void execOuterCmd(Cmd *cmd,int cmdnum);
void setGoon();
int exists(char *cmdname);
void addJob(int pid,char *cmdname);
void rmJob(int pid,siginfo_t *sip,void *noused);
void ctrl_Z();
void ctrl_C();
void fg(int pid);
void bg(int pid);

/*-----------------------------------------×
 |               输入相关函数               |
 ×-----------------------------------------*/
void addCmd(int isBack){
	int len,i=0;
	CmdList[Cmdno] = (Cmd*)malloc(sizeof(Cmd));
	CmdList[Cmdno]->isBack = isBack;
	CmdList[Cmdno]->args = (char**)malloc(sizeof(char*)*MAX_ARG);
	while (ArgList[i]!=NULL) {
		CmdList[Cmdno]->args[i] = (char*)malloc(sizeof(char)*strlen(ArgList[i]));
		strcpy(CmdList[Cmdno]->args[i],ArgList[i]);
		i++;
	}
	//这里仅仅表示有没有重定向，有的话自然就重定向了，没有的话要在后续的处理程序中判断到底是stdin/stdout还是管道输入输出
	CmdList[Cmdno]->input = (inredir==-1)?-1:inredir;
	CmdList[Cmdno]->output = (outredir==-1)?-1:outredir;
	Cmdno++;
}

void clearArgList(){
	int i;
	for (i=0;i<MAX_ARG;i++) ArgList[i]=NULL;
	inredir = outredir = -1;
	Argno = 0;
}


void addHistory(){
	History *now;
	int i=1;
	now = HistList;
	if (now==NULL) {
		HistList = (History*)malloc(sizeof(History));
		now = HistList;
	}
	else {
		while (now->next!=NULL) now = now->next;
		now->next = (History*)malloc(sizeof(History));
		now = now->next;
	}
	now->next = NULL;
	strcpy(now->cmd,CmdList[0]->args[0]);
	while (CmdList[i]!=NULL) {
		strcat(now->cmd,"|");
		strcat(now->cmd,CmdList[i]->args[0]);
		i++;
	}
}

void freeCmdList(){
	int i,j;
	i=j=0;
	while (CmdList[i]!=NULL && i<MAX_CMD) {
		while (CmdList[i]->args[j]!=NULL && j<MAX_ARG) {free(CmdList[i]->args[j]);j++;}
		free(CmdList[i]);
		CmdList[i]=NULL;
		i++;
	}
	Cmdno = 0;
}

void init() {
	int i;
	FILE* fp;
	char path[50];

	//初始化全局变量
	Cmdno = Argno = 0;
	for (i=0;i<MAX_CMD;i++) CmdList[i] = NULL;
	for (i=0;i<MAX_ARG;i++) ArgList[i] = NULL;
	HistList = NULL;
	inredir = outredir = -1;
	fggpid = getpid();

	//初始化环境变量
	fp = fopen("envpath.cfg","r");
	for (i=0;i<MAX_ENVPATH;i++) EnvPath[i]=NULL;
	i=0;
	while (fscanf(fp,"%s",path)>0) {
		EnvPath[i] = (char *)malloc(strlen(path)*sizeof(char));
		strcpy(EnvPath[i],path);
		i++;
	}

	//注册在作业完成的时候调用的函数:rmJob
	struct sigaction action;//sigaction大概是signal action的缩写
    action.sa_sigaction = rmJob;//指定了信号处理的函数
    sigfillset(&action.sa_mask);//将指定的信号集初始化0
    action.sa_flags = SA_SIGINFO;//标志位，表示传递信号参数
    sigaction(SIGCHLD, &action, NULL);//指定了处理信号的行为(action)

	//注册ctrl+Z,ctrl+C信号接收函数
	signal(SIGTSTP,ctrl_Z);
	signal(SIGINT,ctrl_C);
}

/*-----------------------------------------×
 |               执行相关函数               |
 ×-----------------------------------------*/
void execute(){//2016.03.26 11:42测试用
	int i;//循环变量
	char pwd[MAX_STRING_LENGTH];
	int pid = 0;
	
	for (i=0;i<Cmdno;i++) {//for every command in CmdList
		//first inter command
		if (strcmp(CmdList[i]->args[0],"exit")==0) {//cmd:exit,exit the shell
			exit(0);
		}else if (strcmp(CmdList[i]->args[0],"cd")==0) {//cmd:cd
			if (chdir(CmdList[i]->args[1])<0) printf("No such Direcotry:%s\n",CmdList[i]->args[1]);
		}else if (strcmp(CmdList[i]->args[0],"pwd")==0) {//cmd:pwd,print working directory
			getcwd(pwd,100);
			printf("%s\n",pwd);
		}else if (strcmp(CmdList[i]->args[0],"history")==0) {//cmd:history,print history
			printHistory();
		}else if (strcmp(CmdList[i]->args[0],"jobs")==0) {//cmd:jobs,list all jobs
			printJobs();			
		}else if (strcmp(CmdList[i]->args[0],"bg")==0) {//cmd:bg,put a frountgournd process to background
			pid = atoi(CmdList[i]->args[1]);
			if (pid!=0) bg(pid);
			else printf("illegal pid:%s\n",CmdList[i]->args[1]);
		}else if (strcmp(CmdList[i]->args[0],"fg")==0) {//cmd:fg,put a backgournd process to frountground
			pid = atoi(CmdList[i]->args[1]);
			if (pid!=0) fg(pid);
			else printf("illegal pid:%s\n",CmdList[i]->args[1]);
		}else {//outer command
			execOuterCmd(CmdList[i],i);
		}
	}
}


/*-----------------------------------------×
 |               内部命令函数               |
 ×-----------------------------------------*/
void printHistory() {//2016.03.26 11:42测试用
	History *now = HistList;
	
	while (now!=NULL) {
		printf("%s\n",now->cmd);//have to be override
		now = now->next;
	}
}
void printJobs() {
	Job *now = JobList;
	while (now!=NULL) {
		printf("[%d]    %s\n",now->pid,now->cmd);
		now = now->next;
	}
}

void fg(int pid) {
	Job *now = JobList;
	if (JobList==NULL) return;
	
	while (now->next!=NULL && now->pid!=pid) now = now->next;
	if (now->pid!=pid) {
		printf("process of pid:%d not found\n",pid);
		return;
	}
	
	//setpgid(pid,fggpid);
	kill(pid,SIGCONT);
	fgpid = pid;
	return;

}
void bg(int pid) {
	Job *now = JobList;
	if (JobList==NULL) return;
	
	while (now->next!=NULL && now->pid!=pid) now = now->next;
	if (now->pid!=pid) {
		printf("process of pid:%d not found\n",pid);
		return;
	}

	setpgid(pid,0);
	kill(pid,SIGCONT);
	fgpid = 0;
	return;
}

/*-----------------------------------------×
 |               外部命令函数               |
 ×-----------------------------------------*/
void execOuterCmd(Cmd *cmd,int cmdnum) {
	int pid;
	int inredir,outredir;

	if (exists(cmd->args[0])) {
		pid = fork();
		
		if (pid==0) {//fork出子进程来执行命令
			if (cmdnum>0) {
				rename("pipeintmp","pipetmp");
				rename("pipeouttmp","pipeintmp");
				rename("pipetmp","pipeouttmp");
			}
			if (cmdnum>0) {//如果存在管道输入，输入重定向优先级比管道输入高
				if ((inredir = open("pipeintmp",O_RDONLY,S_IRUSR|S_IWUSR))!=-1) {
					if (dup2(inredir,0)==-1) {
						printf("(pipe)dup error.\n");
						return;
					}
				}
			}
			if (cmd->input!=-1) {//存在输入重定向
				if ((inredir = open(cmd->args[cmd->input],O_RDONLY,S_IRUSR|S_IWUSR))!=-1) {
					if (dup2(inredir,0)==-1) {
						printf("(input)dup error.\n");
						return;
					}else {//把重定向的参数去掉，不然会传到execv里面去就出错了
						free(cmd->args[cmd->input]);
						cmd->args[cmd->input] = NULL;
					}
				}
				else {
					printf("(input)can't open file error:%s\n",cmd->args[cmd->input]);
					return;
				}
			}
			if (cmd->output!=-1) {//存在输出重定向
				if ((outredir = open(cmd->args[cmd->output],O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR))!=-1) {
					if (dup2(outredir,1)==-1) {
						printf("(output)dup error.\n");
						return;
					}else {	//把重定向的参数去掉，不然会传到execv里面去就出错了
						free(cmd->args[cmd->output]);
						cmd->args[cmd->output] = NULL;
					}
				}
				else {
					printf("(output)can't open file error:%s\n",cmd->args[cmd->output]);
					return;
				}
			}
			if (cmdnum+1<Cmdno) {//存在输出管道，管道输出比重定向优先级高
				if ((outredir = open("pipeouttmp",O_WRONLY|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR))!=-1) {
					if (dup2(outredir,1)==-1) {
						printf("(pipe)dup error.\n");
						return;
					}
				}
			}
		
			addJob(getpid(),cmd->args[0]);
			if (cmd->isBack) setpgid(0,0);
			
			execv(CmdBuff,cmd->args);//执行命令
		}else {//父进程
			if (cmd->isBack==0) {//前台运行，那么等待运行结束
				fgpid = pid;
				sleep(1);//wait for child process begin
				waitpid(fgpid,NULL,WUNTRACED);//wait for child process end
				fgpid = 0;
			}
		}
	}else printf("Command not found:%s\n",cmd->args[0]);
}

int exists(char *cmdname) {
	char pathcmd[MAX_STRING_LENGTH];
	int i;
	if (strlen(cmdname)>2 && cmdname[0]=='.' && cmdname[1]=='/') {
		if (access(cmdname,F_OK)==0) {
			strcpy(CmdBuff,cmdname);
			return 1;
		}
	}else for (i=0;i<MAX_ENVPATH;i++) {
		if (EnvPath[i]==NULL) break;
		strcpy(pathcmd,EnvPath[i]);
		strcat(pathcmd,cmdname);
		if (access(pathcmd,F_OK)==0) {
			strcpy(CmdBuff,pathcmd);
			return 1;
		}
	}

	return 0;
}
void setGoon() {goon=1;}

/*-----------------------------------------×
 |               作业相关函数               |
 ×-----------------------------------------*/
void addJob(int pid,char *cmdname) {
	Job *now = JobList;
	
	if (JobList==NULL) {//add first job
		JobList = (Job*)malloc(sizeof(Job));
		JobList->pid = pid;
		strcpy(JobList->cmd,cmdname);
		JobList->state = STATE_RUN;
		JobList->next = NULL;
		return;
	}
	
	while (now->next!=NULL) now = now->next;
	
	//add job to JobList
	now->next = (Job*)malloc(sizeof(Job));
	now = now->next;
	now->pid = pid;
	strcpy(now->cmd,cmdname);
	now->state = STATE_RUN;
	now->next = NULL;
	return;
}
void rmJob(int pid,siginfo_t *sip,void *noused) {
	Job *now = JobList;
	Job *tmp;
	if (JobList==NULL) return;
	if (JobList->pid == pid) {
		JobList = JobList->next;
		free(now);
		return;
	}
	
	while (now->next!=NULL) {
		if ((now->next->pid)==pid) {
			tmp = now->next;
			now->next = now->next->next;
			free(tmp);
			return;
		}
		now = now->next;
	}
	return;
}

/*-----------------------------------------×
 |             键盘信号相关函数             |
 ×-----------------------------------------*/
void ctrl_Z(){
	Job *now;
	now = JobList;
	
	if (fgpid==0) return;
	

	if (JobList==NULL) {
		addJob(fgpid,CmdBuff);
		now = JobList;
	}
	else {
		while (now->next!=NULL && now->pid!=fgpid) now = now->next;
		if (now->pid!=fgpid) {
			addJob(fgpid,CmdBuff);
			now = now->next;
		}
	}

	now->state = STATE_HANG;
	printf("\n[%d]    %s is hang.\n",now->pid,now->cmd);

	kill(fgpid,SIGSTOP);
	fgpid = 0;
	
}
void ctrl_C(){
	Job *now = JobList;
	char cmdname[MAX_STRING_LENGTH];

	if (fgpid==0) return;

	strcpy(cmdname,CmdBuff);

	if (JobList!=NULL) {
		while (now->next!=NULL && now->pid!=fgpid) now = now->next;
		if (now->pid==fgpid) {
			strcpy(cmdname,now->cmd);
			rmJob(fgpid,NULL,NULL);
		}
	}
	
	printf("\n[%d]    %s is killed.\n",fgpid,cmdname);

	kill(fgpid,SIGKILL);
	fgpid = 0;
}

#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/termios.h>

#include "global.h"
#define DEBUG
char *envPath[10], cmdBuff[40];  //外部命令的存放路径及读取外部命令的缓冲空间
History history;                 //历史命令
Job *head = NULL;                //作业头指针
pid_t fgPid;                     //当前前台作业的进程号

/*******************************************************
                  工具以及辅助方法
********************************************************/
/*判断命令是否存在*/
int exists(char *cmdFile){
    int i = 0;
    if((cmdFile[0] == '/' || cmdFile[0] == '.') && access(cmdFile, F_OK) == 0){ //命令在当前目录
        strcpy(cmdBuff, cmdFile);
        return 1;
    }else{  //查找ysh.conf文件中指定的目录，确定命令是否存在
        while(envPath[i] != NULL){ //查找路径已在初始化时设置在envPath[i]中
            strcpy(cmdBuff, envPath[i]);
            strcat(cmdBuff, cmdFile);
            
            if(access(cmdBuff, F_OK) == 0){ //命令文件被找到
                return 1;
            }
            
            i++;
        }
    }
    
    return 0; 
}

/*将字符串转换为整型的Pid*/
int str2Pid(char *str, int start, int end){
    int i, j;
    char chs[20];
    
    for(i = start, j= 0; i < end; i++, j++){
        if(str[i] < '0' || str[i] > '9'){
            return -1;
        }else{
            chs[j] = str[i];
        }
    }
    chs[j] = '\0';
    
    return atoi(chs);
}

/*调整部分外部命令的格式*/
void justArgs(char *str){
    int i, j, len;
    len = strlen(str);
    
    for(i = 0, j = -1; i < len; i++){
        if(str[i] == '/'){
            j = i;
        }
    }

    if(j != -1){ //找到符号'/'
        for(i = 0, j++; j < len; i++, j++){
            str[i] = str[j];
        }
        str[i] = '\0';
    }
}


/*释放环境变量空间*/
void release(){
    int i;
    for(i = 0; strlen(envPath[i]) > 0; i++){
        free(envPath[i]);
    }
}

/*******************************************************
                  信号以及jobs相关
********************************************************/
/*添加新的作业*/
Job* addJob(pid_t pid){
    Job *now = NULL, *last = NULL, *job = (Job*)malloc(sizeof(Job));
    
	//初始化新的job
    job->pid = pid;
    strcpy(job->cmd, inputBuff);
    strcpy(job->state, RUNNING);
    job->next = NULL;
    
    if(head == NULL){ //若是第一个job，则设置为头指针
        head = job;
    }else{ //否则，根据pid将新的job插入到链表的合适位置
		now = head;
		while(now != NULL && now->pid < pid){
			last = now;
			now = now->next;
		}
        last->next = job;
        job->next = now;
    }
    
    return job;
}

/*移除一个作业*/
void rmJob(pid_t pid){
    Job *now = NULL, *last = NULL;
    
    

    now = head;
	while(now != NULL && now->pid < pid){
		last = now;
		now = now->next;
	}
    
    if(now == NULL){ //作业不存在，则不进行处理直接返回
        return;
    }
    
	//开始移除该作业
    if(now == head){
        head = now->next;
    }else{
        last->next = now->next;
    }
    
    free(now);
}
void sg_chld(int sig, siginfo_t *sip, void* noused){
	pid_t pid;
	sigset_t sset;
	Job *now = NULL;
	now = head;

	fflush(stderr);
	if(sig != SIGCHLD)
		return;
	pid = sip->si_pid;
	sigfillset(&sset);
	sigdelset(&sset,SIGCHLD);
	switch(sip->si_code){
		case CLD_EXITED:
		case CLD_KILLED:
		case CLD_DUMPED:
			//fprintf(stderr,",,[%d] closed\n",sip->si_pid);
			if(fgPid == pid || waitpid(fgPid,NULL,WNOHANG)>0){//fgPid  ==getpgid(pid)){
				rmJob(fgPid);
		//	}
		//	if(fgPid == pid){
				tcsetpgrp(STDIN_FILENO,getpid());
				fgPid= 0;
			}
			//else
			//	killpg(-fgPid,SIGKILL);

			while(waitpid(-1,NULL,WNOHANG)>0);
			return;
		case CLD_CONTINUED:
			if(fgPid == getpgid(pid)){
				while(now !=NULL&& now->pid != fgPid)
					now = now->next;
				if(!now)
					return;
				strcpy(now->state,RUNNING);
				now->cmd[strlen(now->cmd)] = '\0';
    			printf("\n[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);
				rmJob(fgPid);
			}
			while(fgPid){
				sigsuspend(&sset);
			}
			return;
		case CLD_STOPPED:
		//	fprintf(stderr,"!![%d] stopped\n",pid);
		//	if(pid != getpgid(pid))
		//		break;
			//fprintf(stderr,",,[%d] stopped\n",sip->si_pid);
			tcsetpgrp(STDIN_FILENO,getpid());
			break;
		default:
			fprintf(stderr,"What condition!!!\n");
			return;
	}
    
    if(fgPid == 0){ //前台没有作业则直接返回
        return;
    }
    
	while(now != NULL && now->pid != fgPid)
		now = now->next;
    
    fgPid = 0;
    if(now == NULL){ //未找到前台作业，则根据fgPid添加前台作业
        now = addJob(getpgid(pid));
    }
    
	//修改前台作业的状态及相应的命令格式，并打印提示信息
    strcpy(now->state, STOPPED); 
    now->cmd[strlen(now->cmd)] = '&';
    now->cmd[strlen(now->cmd) + 1] = '\0';
    printf("\n[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);

}

/*fg命令*/
void fg_exec(int pid){    
    Job *now = NULL; 
	int i;
	sigset_t sset;
	sigfillset(&sset);
	sigdelset(&sset,SIGCHLD);
    
    //SIGCHLD信号产生自此函数
    
	//根据pid查找作业
    now = head;
	while(now != NULL && now->pid != pid)
		now = now->next;
    
    if(now == NULL){ //未找到作业
        printf("pid为 %d 的作业不存在！\n", pid);
        return;
    }

    //记录前台作业的pid，修改对应作业状态
    fgPid = now->pid;
    strcpy(now->state, RUNNING);
    
    i = strlen(now->cmd) - 1;
    while(i >= 0 && now->cmd[i] != '&')
		i--;
    now->cmd[i] = '\0';
    
    printf("%s\n", now->cmd);
	tcsetpgrp(STDIN_FILENO,getpgid(now->pid));
	killpg(getpgid(now->pid),SIGCONT);
	while(fgPid)
		sigsuspend(&sset);
	fprintf(stderr,"out fg\n");
}

/*bg命令*/
void bg_exec(int pid){
    Job *now = NULL;
    
    //SIGCHLD信号产生自此函数
    
	fgPid = 0;
	//根据pid查找作业
	now = head;
    while(now != NULL && now->pid != pid)
		now = now->next;
    
    if(now == NULL){ //未找到作业
        printf("pid为 %d 的作业不存在！\n", pid);
        return;
    }
    
    strcpy(now->state, RUNNING); //修改对象作业的状态
    printf("\n[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);
    
    kill(now->pid, SIGCONT); //向对象作业发送SIGCONT信号，使其运行
}

/*******************************************************
                    命令历史记录
********************************************************/
void addHistory(char *cmd){
    if(history.end == -1){ //第一次使用history命令
        history.end = 0;
        strcpy(history.cmds[history.end], cmd);
        return;
	}
    
    history.end = (history.end + 1)%HISTORY_LEN; //end前移一位
    strcpy(history.cmds[history.end], cmd); //将命令拷贝到end指向的数组中
    
    if(history.end == history.start){ //end和start指向同一位置
        history.start = (history.start + 1)%HISTORY_LEN; //start前移一位
    }
}

/*******************************************************
                     初始化环境
********************************************************/
/*通过路径文件获取环境路径*/
void getEnvPath(int len, char *buf){
    int i, j,  pathIndex = 0, temp;
    char path[40];
    
    for(i = 0, j = 0; i < len; i++){
        if(buf[i] == ':'){ //将以冒号(:)分隔的查找路径分别设置到envPath[]中
            if(path[j-1] != '/'){
                path[j++] = '/';
            }
            path[j] = '\0';
            j = 0;
            
            temp = strlen(path);
            envPath[pathIndex] = (char*)malloc(sizeof(char) * (temp + 1));
            strcpy(envPath[pathIndex], path);
            
            pathIndex++;
        }else{
            path[j++] = buf[i];
        }
    }
    
    envPath[pathIndex] = NULL;
}

/*初始化操作*/
void init(){
    int fd, len;
    char c, buf[80];

	//打开查找路径文件ysh.conf
    if((fd = open("ysh.conf", O_RDONLY, 660)) == -1){
        perror("init environment failed\n");
        exit(1);
    }
    
	//初始化history链表
    history.end = -1;
    history.start = 0;
    
    len = 0;
	//将路径文件内容依次读入到buf[]中
    while(read(fd, &c, 1) != 0){ 
        buf[len++] = c;
    }
    buf[len] = '\0';

    //将环境路径存入envPath[]
    getEnvPath(len, buf); 
    
    //注册信号
    struct sigaction action;
    action.sa_sigaction = sg_chld;
    sigfillset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;
    sigaction(SIGCHLD, &action, NULL);
	signal(SIGTSTP,SIG_IGN);
	signal(SIGINT,SIG_IGN);
}

/*******************************************************
                      命令解析
********************************************************/
SimpleCmd* handleSimpleCmdStr(int begin, int end){
    int i, j, k;
    int fileFinished; //记录命令是否解析完毕
    char buff[10][40], inputFile[30], outputFile[30], *temp = NULL;
    SimpleCmd *cmd = (SimpleCmd*)malloc(sizeof(SimpleCmd));
    
	//默认为非后台命令，输入输出重定向为null
    cmd->isBack = 0;
    cmd->input = cmd->output = NULL;

	cmd->next = NULL;

	cmd->pipeIn = 0;
	cmd->pipeOut = 0;
    //初始化相应变量
    for(i = begin; i<10; i++){
        buff[i][0] = '\0';
    }
    inputFile[0] = '\0';
    outputFile[0] = '\0';
    
    i = begin;
	//跳过空格等无用信息
    while(i < end && (inputBuff[i] == ' ' || inputBuff[i] == '\t')){
        i++;
    }
    
    k = 0;
    j = 0;
    fileFinished = 0;
    temp = buff[k]; //以下通过temp指针的移动实现对buff[i]的顺次赋值过程
    while(i < end){
		/*根据命令字符的不同情况进行不同的处理*/
        switch(inputBuff[i]){ 
            case ' ':
            case '\t': //命令名及参数的结束标志
                temp[j] = '\0';
                j = 0;
                if(!fileFinished){
                    k++;
                    temp = buff[k];
                }
                break;

            case '<': //输入重定向标志
                if(j != 0){
		    //此判断为防止命令直接挨着<符号导致判断为同一个参数，如果ls<sth
                    temp[j] = '\0';
                    j = 0;
                    if(!fileFinished){
                        k++;
                        temp = buff[k];
                    }
                }
                temp = inputFile;
                fileFinished = 1;
                i++;
                break;
                
            case '>': //输出重定向标志
                if(j != 0){
                    temp[j] = '\0';
                    j = 0;
                    if(!fileFinished){
                        k++;
                        temp = buff[k];
                    }
                }
                temp = outputFile;
                fileFinished = 1;
                i++;
                break;
                
            case '&': //后台运行标志
                if(j != 0){
                    temp[j] = '\0';
                    j = 0;
                    if(!fileFinished){
                        k++;
                        temp = buff[k];
                    }
                }
                cmd->isBack = 1;
                fileFinished = 1;
                i++;
                break;
                
            default: //默认则读入到temp指定的空间
                temp[j++] = inputBuff[i++];
                continue;
		}
        
		//跳过空格等无用信息
        while(i < end && (inputBuff[i] == ' ' || inputBuff[i] == '\t')){
            i++;
        }
	}
    
    if(inputBuff[end-1] != ' ' && inputBuff[end-1] != '\t' && inputBuff[end-1] != '&'){
        temp[j] = '\0';
        if(!fileFinished){
            k++;
        }
    }
    
	//依次为命令名及其各个参数赋值
    cmd->args = (char**)malloc(sizeof(char*) * (k + 1));
    cmd->args[k] = NULL;
    for(i = 0; i<k; i++){
        j = strlen(buff[i]);
        cmd->args[i] = (char*)malloc(sizeof(char) * (j + 1));   
        strcpy(cmd->args[i], buff[i]);
    }
    
	//如果有输入重定向文件，则为命令的输入重定向变量赋值
    if(strlen(inputFile) != 0){
        j = strlen(inputFile);
        cmd->input = (char*)malloc(sizeof(char) * (j + 1));
        strcpy(cmd->input, inputFile);
    }

    //如果有输出重定向文件，则为命令的输出重定向变量赋值
    if(strlen(outputFile) != 0){
        j = strlen(outputFile);
        cmd->output = (char*)malloc(sizeof(char) * (j + 1));   
        strcpy(cmd->output, outputFile);
    }
    #ifdef DEBUG
    printf("****\n");
    printf("isBack: %d\n",cmd->isBack);
    	for(i = 0; cmd->args[i] != NULL; i++){
    		printf("args[%d]: %s\n",i,cmd->args[i]);
	}
    printf("input: %s\n",cmd->input);
    printf("output: %s\n",cmd->output);
    printf("****\n");
    #endif
    return cmd;
}

/*******************************************************
                      命令执行
********************************************************/
/*执行外部命令*/
void execOuterCmd(SimpleCmd *cmd){
    pid_t pid;
	static int pgid = 0;
    int pipeIn, pipeOut;
	static int tmpp;
	int p[2] = {0,0};
    
	if(cmd->next){
		if(pipe(p)){
			fprintf(stderr,"Pipe failed\n");
			goto EXIT;
		}
		cmd->pipeOut = p[1];
		cmd->next->pipeIn = p[0];
	}
    if(exists(cmd->args[0])){ //命令存在

        if((pid = fork()) < 0){
            perror("fork failed");
			goto EXIT;
        }
        
        if(pid == 0){ //子进程
			signal(SIGCHLD,SIG_DFL);
			signal(SIGINT,SIG_DFL);
			signal(SIGTSTP,SIG_DFL);
			if(!pgid)
				pgid = getpid();
			setpgid(getpid(),pgid);
			if(cmd->pipeIn){
				dup2(cmd->pipeIn,0);
				close(cmd->pipeIn);
				fprintf(stderr,"%d's in is %d\n",getpid(),cmd->pipeIn);
			}
			if(cmd->pipeOut){
				dup2(cmd->pipeOut,1);
				close(p[1]);
				close(p[0]);
				fprintf(stderr,"%d's out is %d\n",getpid(),cmd->pipeOut);
			}
            if(cmd->input != NULL){ //存在输入重定向
                if((pipeIn = open(cmd->input, O_RDONLY, S_IRUSR|S_IWUSR)) == -1){
                    printf("不能打开文件 %s！\n", cmd->input);
                    return;
                }
                if(dup2(pipeIn, 0) == -1){
                    printf("重定向标准输入错误！\n");
                    return;
                }
            }
            
            if(cmd->output != NULL){ //存在输出重定向
                if((pipeOut = open(cmd->output, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1){
                    printf("不能打开文件 %s！\n", cmd->output);
                    return ;
                }
                if(dup2(pipeOut, 1) == -1){
                    printf("重定向标准输出错误！\n");
                    return;
                }
            }
            
            if(cmd->isBack){ 
                printf("\n[%d]\t%s\t\t%s\n", getpid(), RUNNING, inputBuff);
            }
            
            justArgs(cmd->args[0]);
            if(execv(cmdBuff, cmd->args) < 0){ //执行命令
                printf("execv failed!\n");
                return;
            }
        }
		else{ //父进程
		void execSimpleCmd(SimpleCmd *cmd);//declare
			if(!pgid)
				pgid = pid;
			if(tmpp){
				close(tmpp);
				tmpp = 0;
			}
			if(cmd->next){
			tmpp = p[0];
				close(p[1]);
				execSimpleCmd(cmd->next);
				goto EXIT;
			}
			setpgid(pid,pgid);			//设置子组长
            if(cmd ->isBack){ //后台命令             
                fgPid = 0; //pid置0，为下一命令做准备
                addJob(pgid); //增加新的作业
                
            }else{ //非后台命令
				sigset_t pbm;
                fgPid = getpgid(pid);
				sigfillset(&pbm);
				sigdelset(&pbm,SIGCHLD);

				//，结束子进程等待，阻塞自己
				tcsetpgrp(STDIN_FILENO,pgid);//不用多次使用，STDIN和STDOUT都已被改变
				while(fgPid){
					sigsuspend(&pbm);
				}
            }
		}
    }else{ //命令不存在
		if(cmd->pipeIn)
			close(cmd->pipeIn);
		if(cmd->pipeOut)
			close(cmd->pipeOut);
        printf("找不到命令:%s\n", inputBuff);
    }
EXIT:
	pgid = 0;
}

/*执行命令*/
void execSimpleCmd(SimpleCmd *cmd){
    int i, pid;
    char *temp;
    Job *now = NULL;
    
    if(strcmp(cmd->args[0], "exit") == 0) { //exit命令
        exit(0);
    } else if (strcmp(cmd->args[0], "history") == 0) { //history命令
        if(history.end == -1){
            printf("尚未执行任何命令\n");
            return;
        }
        i = history.start;
        do {
            printf("%s\n", history.cmds[i]);
            i = (i + 1)%HISTORY_LEN;
        } while(i != (history.end + 1)%HISTORY_LEN);
    } else if (strcmp(cmd->args[0], "jobs") == 0) { //jobs命令
        if(head == NULL){
            printf("尚无任何作业\n");
        } else {
            printf("index\tpid\tstate\t\tcommand\n");
            for(i = 1, now = head; now != NULL; now = now->next, i++){
                printf("%d\t%d\t%s\t\t%s\n", i, now->pid, now->state, now->cmd);
            }
        }
    } else if (strcmp(cmd->args[0], "cd") == 0) { //cd命令
        temp = cmd->args[1];
        if(temp != NULL){
            if(chdir(temp) < 0){
                printf("cd; %s 错误的文件名或文件夹名！\n", temp);
            }
        }
    } else if (strcmp(cmd->args[0], "fg") == 0) { //fg命令
        temp = cmd->args[1];
        if(temp != NULL && temp[0] == '%'){
            pid = str2Pid(temp, 1, strlen(temp));
            if(pid != -1){
                fg_exec(pid);
            }
        }else{
            printf("fg; 参数不合法，正确格式为：fg %%<int>\n");
        }
    } else if (strcmp(cmd->args[0], "bg") == 0) { //bg命令
        temp = cmd->args[1];
        if(temp != NULL && temp[0] == '%'){
            pid = str2Pid(temp, 1, strlen(temp));
            
            if(pid != -1){
                bg_exec(pid);
            }
        }
		else{
            printf("bg; 参数不合法，正确格式为：bg %%<int>\n");
        }
    } else{ //外部命令
        execOuterCmd(cmd);
    }
    
    //释放结构体空间
    for(i = 0; cmd->args[i] != NULL; i++){
        free(cmd->args[i]);
        free(cmd->input);
        free(cmd->output);
    }
    free(cmd->args);
    free(cmd);
}

/*******************************************************
                     命令执行接口
********************************************************/
void execute(){
    SimpleCmd *cmd = NULL;
	SimpleCmd *p = NULL;
	int i,l,j;
	l=strlen(inputBuff);
	for(i=0,j=0;i<l;++i){
		if(inputBuff[i]!='|')
			continue;
		if(!cmd)
			cmd = handleSimpleCmdStr(j,i);
		else{
			for(p=cmd;p->next!=NULL;p=p->next);
			p->next = handleSimpleCmdStr(j,i);
		}
		j = i+1;
	}
	if(!cmd)
			cmd = handleSimpleCmdStr(j,i);
		else{
			for(p=cmd;p->next!=NULL;p=p->next);
			p->next = handleSimpleCmdStr(j,i);
		}
	//cmd = handleSimpleCmdStr(0, strlen(inputBuff));
    execSimpleCmd(cmd);
}

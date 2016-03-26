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
#include <dirent.h>

#include "global.h"
#define DEBUG
int goon = 0, ingnore = 0, status, split = 0;       //用于设置signal信号量
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
                return i + 2;
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

/*设置goon*/
void setGoon(){
    goon = 1;
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
void rmJob(int sig, siginfo_t *sip, void* noused){
  
    pid_t pid;
    Job *now = NULL, *last = NULL;
 
    pid = sip->si_pid;
    now = head;
    while(now != NULL && now->pid < pid){
	last = now;
	now = now->next;
    }
    if(now == NULL){ //作业不存在，则不进行处理直接返回
        return;
    }

    if(WIFSTOPPED(status)){

        if(ingnore == 0 && fgPid != 0) {

	strcpy(now->state, STOPPED); 
        int l = strlen(now->cmd);
        now->cmd[l] = '&';
        now->cmd[l + 1] = '\0';
        printf("[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);

	fgPid = 0;

	}
	else {
	  ingnore = 0;
	}
        return;
    }
    if(ingnore == 0){
	//开始移除该作业
    if(now == head){
        head = now->next;
    }else{
        last->next = now->next;
    }
	free(now);
    }
    else{
	ingnore = 0;
	}
    
}

/*组合键命令ctrl+z*/
void ctrl_Z(){

    Job *now = NULL;
    
    if(fgPid == 0){ //前台没有作业则直接返回
        return;
    }
    
    //SIGCHLD信号产生自ctrl+z
    
	now = head;
	while(now != NULL && now->pid != fgPid)
		now = now->next;
    if(now == NULL){ //未找到前台作业，则根据fgPid添加前台作业
        now = addJob(fgPid);
    }
	//修改前台作业的状态及相应的命令格式，并打印提示信息
    strcpy(now->state, STOPPED); 
    int l = strlen(now->cmd);
    now->cmd[l] = '&';
    now->cmd[l + 1] = '\0';
    printf("[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);

	//发送SIGSTOP信号给正在前台运作的工作，将其停止
    
    kill(-fgPid, SIGSTOP);
    tcsetpgrp(0, getpid());
    fgPid = 0;
}

/*组合键命令ctrl+c*/
void ctrl_C(){
	
    Job *now = NULL;
    
    if(fgPid == 0){ //前台没有作业则直接返回
        return;
    }
    
    //SIGCHLD信号产生自ctrl+c
    
	now = head;
	while(now != NULL && now->pid != fgPid)
		now = now->next;
    if(now == NULL){ //未找到前台作业，则根据fgPid添加前台作业
        now = addJob(fgPid);
    }
    //发送SIGKILL信号给正在前台运作的工作
    kill(-fgPid, SIGKILL);
    tcsetpgrp(0, getpid());
    fgPid = 0;
}

/*fg命令*/
void fg_exec(int pid){    
    Job *now = NULL; 
	int i;
    
    //SIGCHLD信号产生自此函数
    
	//根据pid查找作业
    now = head;
	while(now != NULL && now->pid != pid)
		now = now->next;
    
    if(now == NULL){ //未找到作业
        printf("pid: %-7d is not exist!\n", pid);
        return;
    }

    if(strcmp(now->state, RUNNING)){
	ingnore = 1;
    }

    //记录前台作业的pid，修改对应作业状态
    fgPid = now->pid;
    strcpy(now->state, RUNNING);
    
    signal(SIGTSTP, ctrl_Z); //设置signal信号，为下一次按下组合键Ctrl+Z做准备
    i = strlen(now->cmd) - 1;
    while(i >= 0 && now->cmd[i] != '&')
		i--;
    now->cmd[i] = '\0';
    
    printf("%s\n", now->cmd);

    tcsetpgrp(0, fgPid);
    kill(-now->pid, SIGCONT); //向对象作业发送SIGCONT信号，使其运行
    while(waitpid(fgPid, &status, WUNTRACED) == -1);
    tcsetpgrp(0, getpid());
}

/*bg命令*/
void bg_exec(int pid){
    Job *now = NULL;
    
    //SIGCHLD信号产生自此函数
    ingnore = 1;
    
	//根据pid查找作业
	now = head;
    while(now != NULL && now->pid != pid)
		now = now->next;
    
    if(now == NULL){ //未找到作业
        printf("pid: %-7d is not exist!\n", pid);
        return;
    }
    
    strcpy(now->state, RUNNING); //修改对象作业的状态
    printf("[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);
    
    kill(-now->pid, SIGCONT); //向对象作业发送SIGCONT信号，使其运行
	
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
    int i, j, last = 0, pathIndex = 0, temp;
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
    int fd, n, len;
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
    action.sa_sigaction = rmJob;
    sigfillset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;
    sigaction(SIGCHLD, &action, NULL);

    struct sigaction action2;
    action2.sa_handler = SIG_IGN;
    sigaction(SIGTTOU, &action2, NULL);

    signal(SIGTSTP, ctrl_Z);
    signal(SIGINT, ctrl_C);
}

/*******************************************************
                      命令解析
********************************************************/
SimpleCmd* handleSimpleCmdStr(int begin, int end){
    int i, j, k;
    int fileFinished; //记录命令是否解析完毕
    char c, buff[10][40], inputFile[30], outputFile[30], *temp = NULL;
    SimpleCmd *cmd = (SimpleCmd*)malloc(sizeof(SimpleCmd));
    
	//默认为非后台命令，输入输出重定向为null
    cmd->isBack = 0;
    cmd->input = cmd->output = NULL;
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
                    
            case '|': 
                if(j != 0){
                    temp[j] = '\0';
                    j = 0;
                    if(!fileFinished){
                        k++;
                        temp = buff[k];
                    }
                }
		temp[j] = '|';
		j++;
		temp[j] = '\0';
                j = 0;
                if(!fileFinished){
                    k++;
                    temp = buff[k];
                }
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

SimpleCmd* handleSimpleCmdStr0(int begin, int end){
    int i, j, k;
    int fileFinished; //记录命令是否解析完毕
    char c, buff[10][40], inputFile[30], outputFile[30], *temp = NULL;
    SimpleCmd *cmd = (SimpleCmd*)malloc(sizeof(SimpleCmd));

	//默认为非后台命令，输入输出重定向为null
    cmd->isBack = 0;
    cmd->input = cmd->output = NULL;
    
    //初始化相应变量
    for(i = begin; i<10; i++){
        buff[i][0] = '\0';
    }
    inputFile[0] = '\0';
    outputFile[0] = '\0';
    
    i = begin;
	//跳过空格等无用信息
    while(i < end && (inputbuff[i] == ' ' || inputbuff[i] == '\t')){
        i++;
    }
    
    k = 0;
    j = 0;
    fileFinished = 0;
    temp = buff[k]; //以下通过temp指针的移动实现对buff[i]的顺次赋值过程
    while(i < end){
		
        switch(inputbuff[i]){ 
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
                temp[j++] = inputbuff[i++];
                continue;
		}
        
		//跳过空格等无用信息
        while(i < end && (inputbuff[i] == ' ' || inputbuff[i] == '\t')){
            i++;
        }
	}
    
    if(inputbuff[end-1] != ' ' && inputbuff[end-1] != '\t' && inputbuff[end-1] != '&'){
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

    return cmd;
	
}
/*******************************************************
                      命令执行
********************************************************/
int trave_dir(char* path,char filename[][256])
{
    DIR *d;
    int len=0;
    struct dirent *file;
   
    if(!(d = opendir(path)))
    {
        printf("error opendir %s!!!\n",path);
        return -1;
    }
    while((file = readdir(d)) != NULL)
    {
       //把当前目录.，上一级目录..及隐藏文件都去掉，避免死循环遍历目录
        if(strncmp(file->d_name, ".", 1) == 0)
            continue;
        strcpy(filename[len++], file->d_name);//保存遍历到的文件名
       
    }
    closedir(d);
    return len;
}

void execPipe(SimpleCmd *cmd){
    pid_t pid;
    /*int i, j, k, l = strlen(inputBuff), no;*/
    int i, j, k, flag, fd[2], pidpid[2];
    SimpleCmd *cmd1, *cmd2;

    if((pid = fork()) < 0){
	perror("fork failed");
        return;
    }
        
    if(pid == 0) { //子进程

	if(cmd->isBack){ //若是后台运行命令，等待父进程增加作业
		kill(getppid(), SIGUSR1);

	    	signal(SIGUSR1, setGoon); 	
                while(goon == 0) ; //等待父进程SIGUSR1信号，表示作业已加到链表中
                goon = 0; //置0，为下一命令做准备
                
                printf("[%d]\t%s\t\t%s\n", getpid(), RUNNING, inputBuff);

                kill(getppid(), SIGUSR1);
         }

	 setpgid(0, 0);
	 
       	 if(!cmd->isBack){ 
	    kill(getppid(), SIGUSR1);

	    signal(SIGUSR1, setGoon);
            while(goon == 0) ;
                goon = 0;
	 }

	split = 1;

	for(i=0,flag=0,j=0;i<strlen(inputBuff);++i){
		if(flag){
			inputbuff[j++]=inputBuff[i];
		}
		else if(inputBuff[i] == '|'){
			flag=1;
			inputbuff[j]='\0';
			cmd1=handleSimpleCmdStr0(0,strlen(inputbuff));
			j=0;
		}
		else {
			inputbuff[j++]=inputBuff[i];
		}
	}
	inputbuff[j]='\0';
	cmd2=handleSimpleCmdStr0(0,strlen(inputbuff));
	
	if(pipe(fd)<0) printf("Pipe create error/n");
	
	while((pidpid[0]=fork())==-1);
	
	if(!pidpid[0]){
		close(fd[0]);
		close(1);
		dup(fd[1]);
		close(fd[1]);
		execSimpleCmd(cmd1);
	}
	else{
		while((pidpid[1]=fork())==-1);
		if(!pidpid[1]){
			close(fd[1]);
			close(0);
			dup(fd[0]);
			close(fd[0]);
			execSimpleCmd(cmd2);
		}
		else{
			close(fd[0]);
			close(fd[1]);
			waitpid(pidpid[0], &status, WUNTRACED);
			waitpid(pidpid[1], &status, WUNTRACED);
		}
	}

	/* no = 1;
	 i = 0;
	 for(j = 0; j <= l - 1 && inputBuff[j] != '&'; j++) {
	     if(inputBuff[j] == '|') {
	         for(k = 0; i < j; i++) {
		    inputbuff[k++] = inputBuff[i];
                 }
		 inputbuff[k] = '\0';
		 if(no % 2 == 1) {
		     if(no == 1) {
		         strcat(inputbuff, " > y.txt");
		     }
		     else {
			 strcat(inputbuff, " < x.txt > y.txt");
		     }
		 }
		 else {
		     strcat(inputbuff, " < y.txt > x.txt");
		 }
		 SimpleCmd *cmdcmd = handleSimpleCmdStr0(0, strlen(inputbuff));
   		 execSimpleCmd(cmdcmd);
		 no++;
	         i = j + 1;
	     }
	 }
	 for(k = 0; i < j; i++) {
	     inputbuff[k++] = inputBuff[i];
         }
	 inputbuff[k] = '\0';
	 if(no % 2 == 1) {
	     strcat(inputbuff, " < x.txt");
	 }
	 else {
             strcat(inputbuff, " < y.txt");
	 } 
	 SimpleCmd *cmdcmd = handleSimpleCmdStr0(0, strlen(inputbuff));
   	 execSimpleCmd(cmdcmd);*/
	 split = 0;
	 _exit(0);
    }
    else { //父进程

      	if(cmd ->isBack){ //后台命令 
		signal(SIGUSR1, setGoon);  
		while(goon == 0) ;
                goon = 0;
          
                fgPid = 0; //pid置0，为下一命令做准备
                addJob(pid); //增加新的作业
                kill(pid, SIGUSR1); //子进程发信号，表示作业已加入
                
                //等待子进程输出
                signal(SIGUSR1, setGoon);
                while(goon == 0) ;
                goon = 0;
            }else{ //非后台命令
		signal(SIGUSR1, setGoon);  
		while(goon == 0) ;
                    goon = 0;

		tcsetpgrp(0, pid);
		fgPid = pid;
		addJob(pid); //增加新的作业

		kill(pid, SIGUSR1);
		waitpid(pid, &status, WUNTRACED);
		tcsetpgrp(0, getpid());
            }

    }
   
}

void dealWildcard(SimpleCmd *cmd){
	int i,sum,f1=0,f2=0,j,num,mark1[100],mark2[100],mark3[100],mark4[100],num1=0,num2=0,k,f3,f4,flag=0;
	char filename[256][256],*cwd;
	SimpleCmd *dupcmd = (SimpleCmd*)malloc(sizeof(SimpleCmd));
	dupcmd->isBack=cmd->isBack;
	dupcmd->input=NULL;
	dupcmd->output=NULL;
	for(i=0;cmd->args[i] != NULL;++i){
		for(j=0;j<strlen(cmd->args[i]);++j){
			if(cmd->args[i][j]=='*'){
				mark1[num1]=i;
				mark2[num1++]=j;
				f1=1;
				break;
			}
			if(cmd->args[i][j]=='?'){
				mark3[num2]=i;
				mark4[num2++]=j;
				f2=1;
				break;
			}
		}
	}
	
	sum=i;
	dupcmd->args = (char**)malloc(sizeof(char*) * (sum+50));
	
	for(i=0;cmd->args[i]!=NULL;++i){
		dupcmd->args[i] = (char*)malloc(sizeof(char) * (strlen(cmd->args[i]) + 1)); 
		strcpy(dupcmd->args[i],cmd->args[i]);
	}
	dupcmd->args[i]=NULL;
	
	if(f1){
		cwd=(char *)malloc(80);
		getcwd(cwd,80);
		num=trave_dir(cwd,filename);
		
		for(i=0;i<num1;++i){
			f4=0;
			for(j=0;j<num;++j){
				f3=1;
				for(k=0;k<mark2[i];++k){
					if(dupcmd->args[mark1[i]][k]!=filename[j][k]){
						f3=0;
						break;
					}
				}
				
				if(f3){
					f4=1;
					flag=1;
					dupcmd->args[sum] = (char*)malloc(sizeof(char) * (strlen(filename[j]) + 1));   
					strcpy(dupcmd->args[sum],filename[j]);
					dupcmd->args[++sum] = NULL;
				}
			}
			if(f4){
				for(j=mark1[i];dupcmd->args[j+1]!=NULL;++j){
					strcpy(dupcmd->args[j],dupcmd->args[j+1]);
				}
				dupcmd->args[j]=NULL;
				sum--;
				for(j=i+1;j<num1;++j){
					mark1[j]--;
				}
				for(j=0;j<num2;++j){
					if(mark3[j]>mark1[i]) mark3[j]--;
				}
			}
		}
	}
	
	if(f2){
		cwd=(char *)malloc(80);
		getcwd(cwd,80);
		num=trave_dir(cwd,filename);
		for(i=0;i<num2;++i){
			f4=0;
			for(j=0;j<num;++j){
				f3=1;
				for(k=0;k<mark4[i];++k){
					if(dupcmd->args[mark3[i]][k]!=filename[j][k]){
						f3=0;
						break;
					}
				}
				if(f3 && strlen(filename[j])==strlen(dupcmd->args[mark3[i]])){
					f4=1;
					flag=1;
					dupcmd->args[sum] = (char*)malloc(sizeof(char) * (strlen(filename[j]) + 1));   
					strcpy(dupcmd->args[sum],filename[j]);
					dupcmd->args[++sum] = NULL;
				}
			}
			if(f4){
				for(j=mark3[i];dupcmd->args[j+1]!=NULL;++j){
					strcpy(dupcmd->args[j],dupcmd->args[j+1]);
				}
				dupcmd->args[j]=NULL;
				sum--;
				for(j=i+1;j<num1;++j) mark3[j]--;
			}
		}
	}
	if(flag){
		execSimpleCmd(dupcmd);
	}
	else{ 
		execOuterCmd(cmd);
	}
}

/*执行外部命令*/
void execOuterCmd(SimpleCmd *cmd){
    pid_t pid;
    int pipeIn, pipeOut;

    if(exists(cmd->args[0])){ //命令存在

        if((pid = fork()) < 0){
            perror("fork failed");
            return;
        }
        
        if(pid == 0){ //子进程
            if(cmd->input != NULL){ //存在输入重定向
                if((pipeIn = open(cmd->input, O_RDONLY, S_IRUSR|S_IWUSR)) == -1){
                    printf("Can't open file %s!\n", cmd->input);
                    return;
                }
                if(dup2(pipeIn, 0) == -1){
                    printf("Redirect standard input error!\n");
                    return;
                }
            }
            
            if(cmd->output != NULL){ //存在输出重定向
                if((pipeOut = open(cmd->output, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1){
                    printf("Can't open file %s!\n", cmd->output);
                    return ;
                }
                if(dup2(pipeOut, 1) == -1){
                    printf("Redirect standard output error!\n");
                    return;
                }
            }
            
            if(cmd->isBack){ //若是后台运行命令，等待父进程增加作业
		kill(getppid(), SIGUSR1);

                signal(SIGUSR1, setGoon);	
                while(goon == 0) ; //等待父进程SIGUSR1信号，表示作业已加到链表中
                goon = 0; //置0，为下一命令做准备
                
                printf("[%d]\t%s\t\t%s\n", getpid(), RUNNING, inputBuff);

                kill(getppid(), SIGUSR1);
            }
            
            justArgs(cmd->args[0]);

	    setpgid(0, 0);
		
	    if(!cmd->isBack){ 
	    	kill(getppid(), SIGUSR1);

	    	signal(SIGUSR1, setGoon);
            	while(goon == 0) ;
                    goon = 0;
	    }
            if(execv(cmdBuff, cmd->args) < 0){ //执行命令
                printf("execv failed!\n");
                _exit(0);
            }
        }
	else{ //父进程
            if(cmd ->isBack){ //后台命令 
		signal(SIGUSR1, setGoon);
		while(goon == 0) ;
                goon = 0;
          
                fgPid = 0; //pid置0，为下一命令做准备
                addJob(pid); //增加新的作业
                kill(pid, SIGUSR1); //子进程发信号，表示作业已加入
                
                //等待子进程输出
                signal(SIGUSR1, setGoon);
                while(goon == 0) ;
                goon = 0;
            }
	    else{ //非后台命令
		signal(SIGUSR1, setGoon);  
		while(goon == 0) ;
                    goon = 0;

		tcsetpgrp(0, pid);
	        fgPid = pid;
		addJob(pid); //增加新的作业

		kill(pid, SIGUSR1);
		waitpid(pid, &status, WUNTRACED);
		tcsetpgrp(0, getpid());
            }
	}
    } else{ //命令不存在
        printf("Command is not exist %-15s\n", inputBuff);
    }
}

/*执行命令*/
void execSimpleCmd(SimpleCmd *cmd){
    int i, j, pid, isPipe, hasWildCard;
    char *temp;
    Job *now = NULL;
    isPipe = 0;
    hasWildCard = 0;

    for(i = 0; i < strlen(inputBuff); i++){
        if(inputBuff[i] == '|'){
	    isPipe++;
	}
    }

    for(i = 0; i < strlen(inputBuff); i++){
        if(inputBuff[i] == '*' || inputBuff[i] == '?'){
	    hasWildCard++;
	}
    }

    if(isPipe && (split == 0)){
	execPipe(cmd);
    } else if(hasWildCard){
	dealWildcard(cmd);
    } else if(strcmp(cmd->args[0], "exit") == 0){ //exit命令
        exit(0);
    } else if (strcmp(cmd->args[0], "history") == 0){ //history命令
        if(history.end == -1){
            printf("There is no command\n");
            return;
        }
        i = history.start;
        do {
            printf("%s\n", history.cmds[i]);
            i = (i + 1)%HISTORY_LEN;
        } while(i != (history.end + 1)%HISTORY_LEN);
    } else if (strcmp(cmd->args[0], "jobs") == 0){ //jobs命令
        if(head == NULL){
            printf("There is no work\n");
        } else {
            printf("index\tpid\tstate\t\tcommand\n");
            for(i = 1, now = head; now != NULL; now = now->next, i++){
                printf("%d\t%d\t%s\t\t%s\n", i, now->pid, now->state, now->cmd);
            }
        }
    } else if (strcmp(cmd->args[0], "cd") == 0){ //cd命令
        temp = cmd->args[1];
        if(temp != NULL){
            if(chdir(temp) < 0){
                printf("cd; %s Wrong file or folder!\n", temp);
            }
        }
    } else if (strcmp(cmd->args[0], "fg") == 0){ //fg命令
        temp = cmd->args[1];
        if(temp != NULL && temp[0] == '%'){
            pid = str2Pid(temp, 1, strlen(temp));
            if(pid != -1){
                fg_exec(pid);
            }
        }else{
            printf("fg; Parameter is not valid, the correct format is: fg %<int>\n");
        }
    } else if (strcmp(cmd->args[0], "bg") == 0){ //bg命令
        temp = cmd->args[1];
        if(temp != NULL && temp[0] == '%'){
            pid = str2Pid(temp, 1, strlen(temp));
            
            if(pid != -1){
                bg_exec(pid);
            }
        }
		else{
            printf("bg; Parameter is not valid, the correct format is: bg %<int>\n");
        }
    } else if (strcmp(cmd->args[0], "echo") == 0){ //echo命令
        i = 0; 
	while(inputBuff[i] != 'e')
	   i++;
	i += 4;
        while(inputBuff[i] == ' ')
	   i++;
	for(; i < strlen(inputBuff); i++){
           printf("%c", inputBuff[i]);
    	}
	printf("\n");
    } else if (strcmp(cmd->args[0], "kill") == 0){ //kill命令
	i = 0;
	while(cmd->args[i] != NULL)
	    i++;
        if(i == 2){
	    int a = str2Pid(cmd->args[1], 0, strlen(cmd->args[1]));
	    if(a != -1){
		now = head;
		while(now != NULL && now->pid != a)
			now = now->next;
    		if(now == NULL){ 
        	    printf("Pid is not exist!\n");
    		}
		else{
		    kill(a, 15);
		}
	    }
	    else{
		printf("Format is incorrect!\n");
	    }
	}
	else if(i == 3 && strlen(cmd->args[1]) > 1 && cmd->args[1][0] == '-'){
	    int a = str2Pid(cmd->args[1], 1, strlen(cmd->args[1]));
	    int b = str2Pid(cmd->args[2], 0, strlen(cmd->args[2]));
	    if(a != -1 && b != -1){
	        now = head;
	        while(now != NULL && now->pid != b)
	            now = now->next;
    		if(now == NULL){ 
        	    printf("Pid is not exist!\n");
    		}
		else{
	 	    kill(b, a);
		}
	    }
	    else{
		printf("Format is incorrect!\n");
	    }
	}
	else{
	    printf("Format is incorrect!\n");
	}
    } else if (strcmp(cmd->args[0], "type") == 0){ //type命令
	for(i = 1; cmd->args[i] != NULL; i++){
	    if(exists(cmd->args[i])){
	        if(exists(cmd->args[i]) == 1){
	            printf("%s: is %s\n", cmd->args[i], cmd->args[i]);
		}
		else{
		    printf("%s: is %s\n", cmd->args[i], envPath[exists(cmd->args[i]) - 2]);
		}
	    }
	    else if(strcmp(cmd->args[i], "exit") == 0 ||
		    strcmp(cmd->args[i], "history") == 0 || 
		    strcmp(cmd->args[i], "jobs") == 0 ||
		    strcmp(cmd->args[i], "cd") == 0 ||
		    strcmp(cmd->args[i], "fg") == 0 || 
		    strcmp(cmd->args[i], "bg") == 0 ||
		    strcmp(cmd->args[i], "echo") == 0 ||
	            strcmp(cmd->args[i], "kill") == 0 || 
		    strcmp(cmd->args[i], "type") == 0){
		printf("%s: is in myshell\n", cmd->args[i]);
	    }
	    else{
		printf("bash: type: %s: is not found\n", cmd->args[i]);
	    }
	}
    } else{ //外部命令
        execOuterCmd(cmd);
    }
    
    //释放结构体空间
    for(i = 0; cmd->args[i] != NULL; i++){
        free(cmd->args[i]);
    }
    free(cmd->input);
    free(cmd->output);
 
}

/*******************************************************
                     命令执行接口
********************************************************/
void execute(){
    SimpleCmd *cmd = handleSimpleCmdStr(0, strlen(inputBuff));
    execSimpleCmd(cmd);
}

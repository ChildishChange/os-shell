#include "global.h"

int flag_piped = 0;
extern int pipedes[2];
int goon = 0, ingnore = 0;       //用于设置signal信号量
char *envPath[10], cmdBuff[40];  //外部命令的存放路径及读取外部命令的缓冲空间
History history;                 //历史命令
Job *head = NULL;                //作业头指针
pid_t fgPid;                     //当前前台作业的进程号

/*******************************************************
                  工具以及辅助方法
********************************************************/
/*判断外部命令是否存在*/
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
    int i;
    Job *now = NULL, *last = NULL, *job = (Job*)malloc(sizeof(Job));
    
	//初始化新的job
    job->pid = pid;
    strcpy(job->cmd,cmd->args[0]);
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
    
    if(ingnore == 1){
        ingnore = 0;
        return;
    }
    
    pid = sip->si_pid;

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

/*组合键命令ctrl+z*/
void ctrl_Z(){
    Job *now = NULL;
    
    if(fgPid == 0){ //前台没有作业则直接返回
        return;
    }
    
    //SIGCHLD信号产生自ctrl+z
    ingnore = 1;
    
	now = head;
	while(now != NULL && now->pid != fgPid)
		now = now->next;
    
    if(now == NULL){ //未找到前台作业，则根据fgPid添加前台作业
        now = addJob(fgPid);
    }
    
	//修改前台作业的状态及相应的命令格式，并打印提示信息
    strcpy(now->state, STOPPED); 
    now->cmd[strlen(now->cmd)] = '&';
    now->cmd[strlen(now->cmd) + 1] = '\0';
    printf("[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);
    
	//发送SIGSTOP信号给正在前台运作的工作，将其停止
    kill(fgPid, SIGSTOP);
    fgPid = 0;
}

/*组合键命令ctrl+c*/
void ctrl_C(){
    Job *now = NULL;
    
    if(fgPid == 0){ //前台没有作业则直接返回
        return;
    }
    
    //SIGCINT信号产生自ctrl+c
    ingnore = 1;
    
	now = head;
	while(now != NULL && now->pid != fgPid)
		now = now->next;
    
    if(now == NULL){ //未找到前台作业，则根据fgPid添加前台作业
        now = addJob(fgPid);
    }
    
	//修改前台作业的状态及相应的命令格式，并打印提示信息
    strcpy(now->state, DONE); 
    now->cmd[strlen(now->cmd)] = '&';
    now->cmd[strlen(now->cmd) + 1] = '\0';
    printf("[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);
    
	//发送SIGKILL信号给正在前台运作的工作，将其停止
    kill(fgPid, SIGKILL);
    fgPid = 0;
}

/*fg命令*/
void fg_exec(int pid){    
    Job *now = NULL; 
	int i;
    
    //SIGCHLD信号产生自此函数
    ingnore = 1;
    
	//根据pid查找作业
    now = head;
	while(now != NULL && now->pid != pid)
		now = now->next;
    
    if(now == NULL){ //未找到作业
        printf("pid为%d 的作业不存在！\n", pid);
        return;
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
    kill(now->pid, SIGCONT); //向对象作业发送SIGCONT信号，使其运行
	sleep(1);
    waitpid(fgPid, NULL, 0); //父进程等待前台进程的运行
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
        printf("pid为%d 的作业不存在！\n", pid);
        return;
    }
    
    strcpy(now->state, RUNNING); //修改对象作业的状态
    printf("[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);
    
    kill(now->pid, SIGCONT); //向对象作业发送SIGCONT信号，使其运行
}

/*echo 命令*/
void echo(int argc,char *args[]) { 

	int nflag=0; 

	if(1 < argc && *++args && !strcmp(*args,"-n") ){ 
		nflag=1; 
		++args; 
	}
	else if(!strcmp(*args,"-e")){ 
		++args;
	}
	while(1 < argc && *args){ 
 		(void)printf("%s",*args); 
		if(*++args){ 	
			putchar(' '); 

 		} 
	} 
	if(!nflag){ 
		putchar('\n'); 
	} 
} 

/*test命令*/
int test(int argc,char *args[]){
	int int1=0;
	int int2=0;
	if(argc<1){
		printf("并没有任何输入，我们继续吧～喵\n");
		return 0;
	}	
	else if(strcmp(args[0],"test")==0&&strcmp(args[2],"=")==0){
		// str1==str2?暂时默认字符串不含有空格等空白符
		if(argc!=4){
			printf("输入格式不正确，请按照test str1 =  str2\n");
			return 0;
		}
		else if(strcmp(args[1],args[3])==0)
			return 1;
		else 
			return 0;
	}
	else if(strcmp(args[0],"test")==0&&strcmp(args[2],"!=")==0){
		
		//str1!=str2
		if(argc!=4){
			printf("输入格式不正确，请按照test str1 !=  str2\n");
			return 0;
		}
		else if(strcmp(args[1],args[3])!=0)
			return 1;
		else
			return 0;	
	}
	else if(strcmp(args[0],"test")==0&&strcmp(args[2],"-eq")==0){
		if(argc!=4){
			printf("输入格式不正确，请按照test int1 -eq int2\n");
			return 0;
		}
		else{
			int1=atoi(args[1]);
			int2=atoi(args[3]);
			if(int1==int2)
				return 1;
			else 
				return 0;		
		}
			
	}
	else if(strcmp(args[0],"test")==0&&strcmp(args[2],"-ne")==0){
		if(argc!=4){
			printf("输入格式不正确，请按照test int1 -ne int2\n");
			return 0;
		}
		else{
			int1=atoi(args[1]);
			int2=atoi(args[3]);
			if(int1!=int2)
				return 1;
			else 
				return 0;		
		}
			
	}
	else if(strcmp(args[0],"test")==0&&strcmp(args[2],"-lt")==0){
		if(argc!=4){
			printf("输入格式不正确，请按照test int1 -lt int2\n");
			return 0;
		}
		else{
			int1=atoi(args[1]);
			int2=atoi(args[3]);
			if(int1<int2)
				return 1;
			else 
				return 0;		
		}
			
	}
	else if(strcmp(args[0],"test")==0&&strcmp(args[2],"-le")==0){
		if(argc!=4){
			printf("输入格式不正确，请按照test int1 -le int2\n");
			return 0;
		}
		else{
			int1=atoi(args[1]);
			int2=atoi(args[3]);
			if(int1<=int2)
				return 1;
			else 
				return 0;		
		}
			
	}
	else if(strcmp(args[0],"test")==0&&strcmp(args[2],"-gt")==0){
		if(argc!=4){
			printf("输入格式不正确，请按照test int1 -gt int2\n");
			return 0;
		}
		else{
			int1=atoi(args[1]);
			int2=atoi(args[3]);
			if(int1>int2)
				return 1;
			else 
				return 0;		
		}
			
	}
	else if(strcmp(args[0],"test")==0&&strcmp(args[2],"-ge")==0){
		if(argc!=4){
			printf("输入格式不正确，请按照test int1 -ge int2\n");
			return 0;
		}
		else{
			int1=atoi(args[1]);
			int2=atoi(args[3]);
			if(int1>=int2)
				return 1;
			else 
				return 0;		
		}
			
	}
	else if(strcmp(args[0],"test")==0&&strcmp(args[1],"-n")==0){
		if(argc!=3){
			printf("输入格式不正确，请按照test -n str\n");
			return 0;
		}
		else{
			if(strlen(args[2])!=0)
				return 1;
			else 
				return 0;		
		}
			
	}
	else if(strcmp(args[0],"test")==0&&strcmp(args[1],"-z")==0){
		if(argc!=3){
			printf("输入格式不正确，请按照test -n str\n");
			return 0;
		}
		else{
			if(strlen(args[2])==0)
				return 1;
			else 
				return 0;		
		}
			
	}
	else {
		printf("其他test相关的功能还没有实现QAQ\n");
	}	
		
}
/*type命令*/
void type(int argc ,char *args[]){
    if (strcmp(cmd->args[1], "fg") == 0) {
        printf("%s是our-shell内建指令\n",args[1]);
    } else if (strcmp(cmd->args[1], "bg") == 0) {
        printf("%s是our-shell内建指令\n",args[1]);
    } else if (strcmp(cmd->args[1], "cd") == 0) {
        printf("%s是our-shell内建指令\n",args[1]);
    } else if (strcmp(cmd->args[1], "exit") == 0) {
        printf("%s是our-shell内建指令\n",args[1]);
    } else if (strcmp(cmd->args[1], "history") == 0) {
        printf("%s是our-shell内建指令\n",args[1]);
    } else if (strcmp(cmd->args[1], "jobs") == 0) {
        printf("%s是our-shell内建指令\n",args[1]);
    } else if (strcmp(cmd->args[1], "test") == 0) {
        printf("%s是our-shell内建指令\n",args[1]);
    } else if (strcmp(cmd->args[1], "type") == 0) {
        printf("%s是our-shell内建指令\n",args[1]);
    } else if (strcmp(cmd->args[1], "echo") == 0) {
        printf("%s是our-shell内建指令\n",args[1]);
    } 
    else if(exists(cmd->args[1]))
	printf("%s是外部指令，路径为%s\n",args[1],cmdBuff);
    else
	printf("没有%s指令\n",args[1]);
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
    char path[100];
    
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
    
    cmd = (Command*)malloc(sizeof(Command));
    memset(cmd,'\0',sizeof(Command));
	//打开查找路径文件ysh.conf
    if((fd = open("ysh.conf", O_RDONLY, 660)) == -1){
        perror("环境初始化失败\n");
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
    signal(SIGTSTP, ctrl_Z);
    signal(SIGINT, ctrl_C);
}

/*******************************************************
                      命令执行
********************************************************/
/*执行外部命令*/
void execOuterCmd(Command *cmd,int tempfiledes[]){
    pid_t pid;
    int pipeIn, pipeOut, filedes[2],i,j;
    char **arglist;
    /*把命令名和参数表放进char**类型里面去*/
    arglist = (char**)malloc(sizeof(char*) * (cmd->argc));
    arglist[cmd->argc] = NULL;
    for(i = 0; i<cmd->argc; i++){
        j = strlen(cmd->args[i]);
        arglist[i] = (char*)malloc(sizeof(char) * (j + 1));   
        strcpy(arglist[i], cmd->args[i]);
    }
    
    if(exists(cmd->args[0])){ //命令存在

        if((pid = fork()) < 0){
            perror("创建进程失败\n");
            return;
        }
        
        if(pid == 0){ //子进程
	    if(cmd->isPiped){//如果是管道的输入端，把标准输出重定向到管道的输入端
			//printf("bfr:%d %d\n",pipedes[0],pipedes[1]);
		close(pipedes[0]);
		close(1);
		dup2(pipedes[1],1);
		close(pipedes[1]);
		
	    }

	    if(cmd->aftPipe){//如果前一条指令是管道的输入端，把标准输入重定向到全局变量中保存的管道的输出端
		if(tempfiledes[0]!=0){
			//printf("aft:%d %d\n",tempfiledes[0],tempfiledes[1]);
		    close(tempfiledes[1]);
		    close(0);
    		    dup2(tempfiledes[0],0);
		    close(tempfiledes[0]);
		}
		else{
		    printf("管道出错\n");
		    return;
		}
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

            
            if(cmd->isBack){ //若是后台运行命令，等待父进程增加作业
                signal(SIGUSR1, setGoon); //收到信号，setGoon函数将goon置1，以跳出下面的循环
                while(goon == 0) ; //等待父进程SIGUSR1信号，表示作业已加到链表中
                goon = 0; //置0，为下一命令做准备
                
                //printf("[%d]\t%s\t\t%s\n", getpid(), RUNNING, inputBuff);
                kill(getppid(), SIGUSR1);
            }
            
            justArgs(cmd->args[0]);
			setpgid(0, 0); //Set child process to separate group
            if(execv(cmdBuff, arglist) < 0){ //执行命令
                printf("execv failed!\n");
                exit(0);
            }
        }
		else{ //父进程
            if(cmd ->isBack){ //后台命令             
                fgPid = 0; //pid置0，为下一命令做准备
                addJob(pid); //增加新的作业
		sleep(1);
                kill(pid, SIGUSR1); //子进程发信号，表示作业已加入
                
                //等待子进程输出
                signal(SIGUSR1, setGoon);
                while(goon == 0) ;
                goon = 0;
            }else{ //非后台命令
                fgPid = pid;
                waitpid(pid, NULL, 0);
            }
		    if(flag_piped==1&&cmd->isPiped==0)
			flag_piped = 0;
		}
    }else{ //命令不存在
        printf("找不到命令 15%s\n",cmd->args[0]);
    }
}

/*执行命令*/
void execSimpleCmd(Command *cmd){
    int i, j,pid;
    int filedes[2],stdfiledes[2],tempfiledes[2];
    char *temp;
    char inputBuff[100]={0};
    int pipeIn, pipeOut;
    Job *now = NULL;
    Job *exitnow = NULL;
    char **arglist;
    /*把命令名和参数表放进char**类型里面去*/
    arglist = (char**)malloc(sizeof(char*) * (cmd->argc));
    arglist[cmd->argc] = NULL;
    for(i = 0; i<cmd->argc; i++){
        j = strlen(cmd->args[i]);
        arglist[i] = (char*)malloc(sizeof(char) * (j + 1));   
        strcpy(arglist[i], cmd->args[i]);
    }
    stdfiledes[0] = dup(0);//保存标准的输入和输出，留待之后恢复用
    stdfiledes[1] = dup(1);
    //printf("std:%d %d\n",stdfiledes[0],stdfiledes[1]);

    tempfiledes[0] = pipedes[0];
    tempfiledes[1] = pipedes[1];
    if(cmd->isPiped)
    {
	pipe(filedes);
	pipedes[0] = filedes[0];
	pipedes[1] = filedes[1];
	flag_piped = 1;
    }
    if(strcmp(cmd->args[1], "fg") == 0||strcmp(cmd->args[1], "bg") == 0||strcmp(cmd->args[1], "cd") == 0
	||strcmp(cmd->args[1], "exit") == 0||strcmp(cmd->args[1], "history") == 0||strcmp(cmd->args[1], "jobs") == 0
	||strcmp(cmd->args[1], "test") == 0||strcmp(cmd->args[1], "type") == 0||strcmp(cmd->args[1], "echo") == 0)
    {
	if(cmd->isPiped){//如果是管道的输入端，把标准输出重定向到管道的输入端
	    close(pipedes[0]);
	    close(1);
	    dup2(pipedes[1],1);
	    close(pipedes[1]);
	}
	if(cmd->aftPipe){//如果前一条指令是管道的输入端，把标准输入重定向到全局变量中保存的管道的输出端
	    if(tempfiledes[0]!=0){
		close(tempfiledes[1]);
		close(0);
    		dup2(tempfiledes[0],0);
		close(tempfiledes[0]);
	    }
	    else{
		printf("管道出错\n");
		return;
	    }
	}
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

    if(strcmp(cmd->args[0], "exit") == 0) { //exit命令
	exitnow = head;//输入exit指令后将所有进程中断								
	while(exitnow != NULL){
  	    kill(exitnow->pid, SIGHUP);
	    exitnow = exitnow->next;		
	}
        exit(0);
    } else if (strcmp(cmd->args[0], "history") == 0) { //history命令
        if(history.end == -1){
            printf("尚未执行任何命令\n");
            return;
        }
        i = history.start;
        do {
            printf("%d\t%s\n", i%HISTORY_LEN, history.cmds[i]);
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
    } else if (strcmp(cmd->args[0], "echo") == 0) { //echo命令
        echo(cmd->argc,arglist);
    } else if (strcmp(cmd->args[0], "test") == 0) { //test命令
        test(cmd->argc,arglist);
    } else if (strcmp(cmd->args[0], "type") == 0) { //type命令
        type(cmd->argc,arglist);
    } else{ //外部命令
        execOuterCmd(cmd,tempfiledes);
    }

    dup2(stdfiledes[0],0);
    dup2(stdfiledes[1],1);

    strcpy(inputBuff,cmd->args[0]);
    for(i=1;i<cmd->argc;i++)
    {
	strcat(inputBuff," ");
	strcat(inputBuff,cmd->args[i]);
    }
    if(cmd->input!=NULL)
    {
	strcat(inputBuff," < ");
	strcat(inputBuff,cmd->input);
    }
    if(cmd->output!=NULL)
    {
	strcat(inputBuff," > ");
	strcat(inputBuff,cmd->output);
    }
    if(cmd->isBack)
	strcat(inputBuff," &");
    addHistory(inputBuff);

    //释放结构体空间
    free(cmd->input);
    free(cmd->output);
}

/*******************************************************
                     命令执行接口
********************************************************/
void execute(){
    if(flag_piped)
	cmd->aftPipe = 1;
    if(flag_piped==1&&cmd->isPiped==0)
	flag_piped = 0;
    execSimpleCmd(cmd);
}


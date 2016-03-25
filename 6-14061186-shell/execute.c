#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/termios.h>

#include "global.h"
#define DEBUG
int goon = 0, ii=0, waitFlag=0;       //用于设置signal信号量
char *envPath[10], cmdBuff[40];  //外部命令的存放路径及读取外部命令的缓冲空间
History history;                 //历史命令
Job *head = NULL;                //作业头指针
pid_t fgPid;                     //当前前台作业的进程号
pTask* pHead=NULL;
void (*old_handler1)(int);
void (*old_handler2)(int);
void (*old_handler3)(int);


/*******************************************************
                  工具以及辅助方法
********************************************************/
/*打印提示信息*/
void printLine()
{
    struct passwd *pwd=getpwuid(getuid());

    char hostname[32];
    //char hostname[]="local";
    int hn_len=32;

    if (errFlag||waitFlag) return;
    gethostname(hostname,hn_len);

    printf("\nE!%s@%s:%s$ ", pwd->pw_name, hostname, get_current_dir_name()); //打印提示符信息    
    fflush(stdout);
}                  
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

void doRM(pid_t pid)
{
    Job *now = NULL, *last = NULL;
    now = head;
    while(now != NULL && now->pid < pid){
        last = now;
        now = now->next;
    }
    
    if(now == NULL){ //作业不存在，则不进行处理直接返回
        return;
    }
    
    //printf("rmJob:[%d]\n", sip->si_pid);
    //开始移除该作业
    if(now == head){
        head = now->next;
    }else{
        last->next = now->next;
    }
    waitpid(pid,NULL,WUNTRACED);
    free(now);    
}

/*移除一个作业*/
void rmJob(int sig, siginfo_t *sip, void* noused){
    
    //printf("status:%d code:%d |pid#%d#\n",sip->si_status, sip->si_code, sip->si_pid);

    if (sip->si_status==SIGTSTP&&fgPid==0||sip->si_status==SIGTTOU||sip->si_status==SIGTTIN) switchStatus(sip->si_pid);
    if (sip->si_status==SIGTSTP||sip->si_status==SIGSTOP) switchStatus(sip->si_pid);
    // if (sip->si_status==SIGINT||sip->si_status==SIGKILL||sip->si_status==SIGTERM) switchStatus(sip->si_pid);

//    printf("status[%d]\n",sip->si_status);


    if (sip->si_status==SIGCONT||sip->si_status==SIGTSTP||sip->si_status==SIGSTOP||sip->si_status==SIGTTOU||sip->si_status==SIGTTIN) return;
    
    doRM(sip->si_pid);
}

/*设置后台程序读写挂起状态*/
void switchStatus(int pid){
    Job *now = NULL;
    
    if(pid == 0){ //直接返回
        return;
    }
        
    now = head;
    while(now != NULL && now->pid != pid)
        now = now->next;
    
    if(now == NULL){ //未找到前台作业，则根据fgPid添加前台作业
        now = addJob(pid);
    }
    
    //修改前台作业的状态及相应的命令格式，并打印提示信息
    strcpy(now->state, STOPPED); 
    printf("\n[%d]\t%s\t%s\n", now->pid, now->state, now->cmd);
}

/*组合键命令ctrl+z*/
void ctrl_Z(){
    // Job *now = NULL;
    
    // if(fgPid == 0){ //前台没有作业则直接返回
    //     return;
    // }
        
    // now = head;
    // while(now != NULL && now->pid != fgPid)
    //     now = now->next;
    
    // if(now == NULL){ //未找到前台作业，则根据fgPid添加前台作业
    //     now = addJob(fgPid);
    // }
    
    // //修改前台作业的状态及相应的命令格式，并打印提示信息
    // strcpy(now->state, STOPPED); 
    // // int len=strlen(now->cmd);
    // // now->cmd[len] = '&';
    // // now->cmd[len + 1] = '\0';
    // printf("\n[%d]\t%s\t%s\n", now->pid, now->state, now->cmd);
    
    // fgPid = 0;
}

/*组合键命令ctrl+c*/
void ctrl_C(){
 //    Job *now = NULL;
    
 //    if(fgPid == 0){ //前台没有作业则直接返回
 //        return;
 //    }
        
	// now = head;
	// while(now != NULL && now->pid != fgPid)
	// 	now = now->next;
    
 //    if(now == NULL){ //未找到前台作业，则根据fgPid添加前台作业
 //        now = addJob(fgPid);
 //    }
    
	// //修改前台作业的状态及相应的命令格式，并打印提示信息
 //    strcpy(now->state, KILLED); 
 //    // int len=strlen(now->cmd);
 //    // now->cmd[len] = '&';
 //    // now->cmd[len + 1] = '\0';
 //    printf("\n[%d]\t%s\t%s\n", now->pid, now->state, now->cmd);
    
 //    fgPid = 0;
}

/*处理管道程序中断*/
void dealPipeExit(int sig, siginfo_t *sip, void* noused)
{
    pTask* ptr;
    pid_t pid=sip->si_pid;
    int status=sip->si_status;
    if (sip->si_status==SIGCONT||sip->si_status==SIGTSTP||sip->si_status==SIGSTOP||sip->si_status==SIGTTOU||sip->si_status==SIGTTIN) return;
    for (ptr=pHead;ptr!=NULL;ptr=ptr->next)
    {
        if (ptr->pid==pid&&ptr->status)
        {
            ptr->status=0;
            close(ptr->rp);
            close(ptr->wp);
            break;
        }
    }
    ++ii;
    //printf("[%d]: %d %d\n", pid, ii, status);
}

/*处理管道程序恢复*/
void restartPipe()
{
    pTask* p=pHead;
    for (;p!=NULL;p=p->next)
    {
        if (p->status)
        {
            kill(p->pid,SIGCONT);
        }
    }
}

/*处理管道程序暂停*/
void stopPipe()
{
    pTask* p=pHead;
    for (;p!=NULL;p=p->next)
    {
        if (p->status)
        {
            kill(p->pid,SIGTSTP);
        }
    }
}

/*增加管道任务*/
pTask* addpTask(pTask* g, int rp, int wp, pid_t pid)
{
    pTask* tmp=(pTask*)malloc(sizeof(pTask));
    tmp->pid=pid;
    tmp->rp=rp;
    tmp->wp=wp;
    tmp->status=1;
    tmp->next=g;
    return tmp;
}

/*实现管道v2*/
void doPipe(SimpleCmd *cmd)
{
    pid_t pid;
    int fd[2], pipeIn, pipeOut, cnt, i, piRd=-1;
    char ps[1]="";
    pTask* ptr;
    pHead=NULL;

    struct sigaction action;
    action.sa_sigaction = dealPipeExit;
    sigfillset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;
    sigaction(SIGCHLD, &action, NULL);
    signal(SIGTSTP, old_handler1);
    signal(SIGINT, old_handler2);
    signal(SIGTTOU, old_handler3);    
    signal(SIGCONT, restartPipe);

    SimpleCmd* p=cmd;
    while (p!=NULL)
    {
        if (p->next!=NULL&&(p->output!=NULL||p->next->input!=NULL)) 
        {
            printf("%s\n", "管道定义重复");
            exit(0);
        }
        if (!exists(p->args[0])&&!builtin(p->args[0])) {
            printf("找不到命令 %s\n", p->args[0]);
            exit(0);
        }
        if (p->next!=NULL)
        {
            p->output=ps;
            p->next->input=ps;
        }
        p=p->next;
    }
    
    p=cmd;
    cnt=0;
    ii=0;

    while (p!=NULL)
    {
        if (p->next!=NULL)
        {
            if (pipe(fd)==-1)
            {
                printf("%s\n", "create pipe failed.");
                exit(0);
            }
            //else printf("%s FD0: %d  FD1: %d\n",p->cmd,fd[0],fd[1]);
        }
        else {
            fd[1]=-1;
        }
            pid=fork();
            ++cnt;
            if (pid==0)
                break;
            else {
                pHead=addpTask(pHead,pid,piRd,fd[1]);
                close(ii);
                close(fd[1]);
            }
            piRd = fd[0];
            ii = piRd; 
            p = p->next;
    }
    i=ii=0;
    if (pid>0) 
    {
        //for (i=3;i<cnt*2+3;++i) close(i);
        while (ii<cnt)
        {
        }
        exit(0);
    }
    if (p->next!=NULL) close(fd[0]);
    if (p->input!=NULL)
    {
        if (strlen(p->input)>0)
        {
            if((pipeIn = open(p->input, O_RDONLY, S_IRUSR|S_IWUSR)) == -1){
                printf("不能打开文件 %s！\n", p->input);
                exit(0);
            }
        }
        else
        {
            pipeIn=piRd;           
        }
    }else pipeIn=0;
    if (p->output!=NULL)
    {
        if (strlen(p->output)>0)
        {
            if((pipeOut = open(p->output, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1){
                printf("不能打开文件 %s！\n", p->output);
                exit(0);
            }
        }
        else
        {
            pipeOut=fd[1];          
        }
    }else pipeOut=1;
//        printf("%d %s %d %d %d IN: %s, OUT: %s \n", getpid(), p->args[0], p->argc, pipeIn, pipeOut,p->input, p->output);
//        if (p->args[p->argc]!=NULL) printf("overflow\n");
            if(dup2(pipeIn, 0) == -1){
                printf("重定向标准输入错误！\n");
                exit(0);
            }
            if(dup2(pipeOut, 1) == -1){
                printf("重定向标准输出错误！\n");
                exit(0);
            }
            if (pipeIn!=0) close(pipeIn); 
            if (pipeOut!=1) close(pipeOut); 
        if (!exists(p->args[0])&&!builtin(p->args[0])) 
        {
            printf("不存在的命令:%s\n", p->args[0]);
            exit(0);
        }
        //printf("%s\n", "here");
        if (!builtin(p->args[0])) 
        {
            justArgs(p->args[0]);            
            //printf("%s\n", "here");
            //printf("exec %s %s %d %s\n", p->args[0],p->args[1],getpid(), cmdBuff);
            if(execv(cmdBuff, p->args) < 0){ //执行命令
                printf("execv failed!\n");
                exit(0);
            }
        }       
        else {
            builtinCmd(p,0);
        }
}

/*fg命令*/
void fg_exec(int pid){    
    Job *now = NULL; 
	int i;
        
	//根据pid查找作业
    now = head;
	while(now != NULL && now->pid != pid)
		now = now->next;
    
    if(now == NULL){ //未找到作业
        printf("pid为 %d 的作业不存在！\n", pid);
        return;
    }
    if (getpgid(now->pid)<0)
    {
        doRM(now->pid);
        return;
    }

    //记录前台作业的pid，修改对应作业状态
    fgPid = now->pid;
    strcpy(now->state, RUNNING);
    
  //   i = strlen(now->cmd) - 1;
  //   while(i >= 0 && now->cmd[i] != '&')
		// i--;
  //   now->cmd[i] = '\0';
    
    printf("running %s\n", now->cmd);

    if (tcsetpgrp(0,fgPid)<0)printf("0failed\n");   //交接控制终端
    if (tcsetpgrp(1,fgPid)<0)printf("1failed\n");
    if (tcsetpgrp(2,fgPid)<0)printf("2failed\n");

    kill(now->pid, SIGCONT); //向对象作业发送SIGCONT信号，使其运行
    sleep(1);
    waitpid(fgPid, NULL, WUNTRACED); //父进程等待前台进程的运行

    if (tcsetpgrp(0,getpid())<0)printf("0failed\n");    //收回控制终端
    if (tcsetpgrp(1,getpid())<0)printf("1failed\n");
    if (tcsetpgrp(2,getpid())<0)printf("2failed\n");                
}

/*bg命令*/
void bg_exec(int pid){
    Job *now = NULL;
        
	//根据pid查找作业
	now = head;
    while(now != NULL && now->pid != pid)
		now = now->next;
    
    if(now == NULL){ //未找到作业
        printf("pid为%d 的作业不存在！\n", pid);
        return;
    }
    if (getpgid(now->pid)<0)
    {
        doRM(now->pid);
        return;
    }
    
    strcpy(now->state, RUNNING); //修改对象作业的状态
    printf("\n[%d]\t%s\t%s\n", now->pid, now->state, now->cmd);
    
    kill(now->pid, SIGCONT); //向对象作业发送SIGCONT信号，使其运行
}

/*echo*/
void echo_exec(char *cmd){
    int i = 0;
    for(i=0;cmd[i]!='\0';i++){
        printf("%c",cmd[i]);
    }
}

void type_exec(char *cmd){
    if (strcmp(cmd,"bg")==0){
        printf("bg is a shell builtin");
    }
    else if(strcmp(cmd,"cd")==0){
        printf("cd is a shell builtin");
    }
    else if(strcmp(cmd,"echo")==0){
        printf("echo is a shell builtin");
    }
    else if(strcmp(cmd,"exit")==0){
        printf("exit is a shell builtin");
    }
    else if(strcmp(cmd,"fg")==0){
        printf("fg is a shell builtin");
    }
    else if(strcmp(cmd,"history")==0){
        printf("history is a shell builtin");
    }
    else if(strcmp(cmd,"jobs")==0){
        printf("jobs is a shell builtin");
    }
    else if(strcmp(cmd,"type")==0){
        printf("type is a shell builtin");
    }
    else if(strcmp(cmd,"wait")==0){
        printf("wait is a shell builtin");
    }
    else {
        exists(cmd);
        printf("%s is %s",cmd,cmdBuff);
    }
    printf("\n");
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
    old_handler1 = signal(SIGTSTP, printLine);
    old_handler2 = signal(SIGINT, printLine);
    old_handler3 = signal(SIGTTOU, SIG_IGN);    
}

/*******************************************************
                      命令解析
********************************************************/
//这里的任务转移到YACC进行
/*******************************************************
                      命令执行
********************************************************/
/*执行外部命令*/
void execOuterCmd(SimpleCmd *cmd){
    pid_t pid;
    int pipeIn, pipeOut;

    if (exists(cmd->args[0])||cmd->next!=NULL){ //命令存在


        if((pid = fork()) < 0){
            perror("fork failed");
            return;
        }
        
        if(pid == 0){ //子进程

            if (cmd->next==NULL)
            {
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
            }   

            if(cmd->isBack){ //若是后台运行命令，等待父进程增加作业
                signal(SIGUSR1, setGoon); //收到信号，setGoon函数将goon置1，以跳出下面的循环
                while(goon == 0) 
                {
                } //等待父进程SIGUSR1信号，表示作业已加到链表中
                goon = 0; //置0，为下一命令做准备
                
                kill(getppid(), SIGUSR1);
            }
            else
            {
                signal(SIGUSR1, setGoon); //收到信号，setGoon函数将goon置1，以跳出下面的循环
                while(goon == 0) ; //等待父进程SIGUSR1信号，表示前后台已切换
                goon = 0; //置0，为下一命令做准备
            }
            
            setpgid(0,0);           
            if (cmd->next==NULL)
            {
                justArgs(cmd->args[0]);
                if(execv(cmdBuff, cmd->args) < 0){ //执行命令
                    printf("execv failed!\n");
                    return;
                }
            }
            else doPipe(cmd);
        }
		else{ //父进程
            addJob(pid); //增加新的作业
            if(cmd ->isBack){ //后台命令  
                printf("[%d]\t%s\t%s\n", pid, RUNNING, inputBuff);
                fgPid = 0; //pid置0，为下一命令做准备
                //等待子进程输出
                signal(SIGUSR1, setGoon);
                sleep(1);
                kill(pid, SIGUSR1); //子进程发信号，表示作业已加入
                
                while(goon == 0) 
                {
                }
                goon = 0;
            }else{ //非后台命令
                fgPid = pid;

                if (tcsetpgrp(0,fgPid)<0)printf("0failed\n");   //转移控制
                if (tcsetpgrp(1,fgPid)<0)printf("1failed\n");
                if (tcsetpgrp(2,fgPid)<0)printf("2failed\n");

                sleep(1);
                kill(pid, SIGUSR1); //子进程发信号，表示控制已转移
                
                waitpid(pid, NULL, WUNTRACED);

                if (tcsetpgrp(0,getpid())<0)printf("0failed\n");    //回收控制
                if (tcsetpgrp(1,getpid())<0)printf("1failed\n");
                if (tcsetpgrp(2,getpid())<0)printf("2failed\n");                
            }
		}
    }else{ //命令不存在
        printf("找不到命令 %s\n", inputBuff);
    }    
}

int builtin(char *x)
{
    if (strcmp(x, "history") == 0 ||
        strcmp(x, "exit") == 0 ||
        strcmp(x, "bg") == 0 ||
        strcmp(x, "fg") == 0 ||
        strcmp(x, "cd") == 0 ||
        strcmp(x, "echo") == 0 ||
        strcmp(x, "type") == 0 ||
        strcmp(x, "jobs") == 0 ||
        strcmp(x, "wait") == 0 )
    return 1;
    return 0;
}

int builtinCmd(SimpleCmd *cmd, int needRedir)
{
    int i, pid;
    int pipeIn,pipeOut;
    char *temp;
    Job *now = NULL;

//    printf("%s\n", cmd->args[0]);
    // if (cmd->input!=NULL) printf("%s\n", cmd->input);
    // if (cmd->output!=NULL) printf("%s\n", cmd->output);
    if (needRedir&&cmd->next!=NULL) return 0;
    if (needRedir)
    {
                if(cmd->input != NULL){ //存在输入重定向
                    if((pipeIn = open(cmd->input, O_RDONLY, S_IRUSR|S_IWUSR)) == -1){
                        printf("不能打开文件 %s！\n", cmd->input);
                        return;
                    }
                    if(dup2(pipeIn, 0) == -1){
                        printf("重定向标准输入错误！\n");
                        return;
                    }
                    close(pipeIn);
                }            
                if(cmd->output != NULL){ //存在输出重定向
                    if((pipeOut = open(cmd->output, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1){
                        printf("不能打开文件 %s！\n", cmd->output);
                        return ;
                    }
                    if(dup2(pipeOut, 1) == -1){
                        printf("重定向标准输出错误！\n");
                        return;
                    close(pipeOut);
                    }
                }        
    }
    
    if(strcmp(cmd->args[0], "exit") == 0) { //exit命令
        exit(0);
        return 1;
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
        return 1;
    } else if (strcmp(cmd->args[0], "jobs") == 0) { //jobs命令
        if(head == NULL){
            printf("尚无任何作业\n");
        } else {
            printf("index\tpid\tgid\tstate\t\tcommand\n");
            for(i = 1, now = head; now != NULL; now = now->next, i++){
                printf("%d\t%d\t%d\t%s\t\t%s\n", i, now->pid, getpgid(now->pid), now->state, now->cmd);
            }
        }
        return 1;
    } else if (strcmp(cmd->args[0], "cd") == 0) { //cd命令
        temp = cmd->args[1];
        if(temp != NULL){
            if(chdir(temp) < 0){
                printf("cd; %s 错误的文件名或文件夹名！\n", temp);
            }
        }
        return 1;
    } else if (strcmp(cmd->args[0], "fg") == 0) { //fg命令
        temp = cmd->args[1];
        if(temp != NULL && temp[0] == '%'){
            pid = str2Pid(temp, 1, strlen(temp));
            if(pid != -1){
                fg_exec(pid);
            }
        }else{
            printf("fg; 参数不合法，正确格式为：fg %<int>\n");
        }
        return 1;
    } else if (strcmp(cmd->args[0], "bg") == 0) { //bg命令
        temp = cmd->args[1];
        if(temp != NULL && temp[0] == '%'){
            pid = str2Pid(temp, 1, strlen(temp));
            
            if(pid != -1){
                bg_exec(pid);
            }
        }else{
            printf("bg; 参数不合法，正确格式为：bg %<int>\n");
        }
        return 1;
    } 
    else if (strcmp(cmd->args[0],"wait")==0){
        waitFlag=1;
        while (1)
        {
            i=0;
            for (now=head;now!=NULL;now=now->next)
            if (strcmp(head->state,RUNNING)==0){
                ++i;
                break;

            }//blank
            if (i==0) break;
        }
        waitFlag=0;
        return 1;
    }//wait 
    else if(strcmp(cmd->args[0],"echo")==0){
        int count = 1;
        while(count<cmd->argc){
            echo_exec(cmd->args[count++]);
            printf(" ");
        }
        if (cmd->argc<2){
            printf("echo:illegal input,supposing echo [String]");
        }
        printf("\n");
        return 1;
    }//eho
    else if(strcmp(cmd->args[0],"type")==0){
        type_exec(cmd->args[1]);
        return 1;
    }
    return 0;
}

/*执行命令*/
void execSimpleCmd(SimpleCmd *cmd){
    int sdi = dup(STDIN_FILENO);
    int sdo = dup(STDOUT_FILENO);
    if (builtinCmd(cmd,1)) 
    {
        dup2(sdo,STDOUT_FILENO);
        dup2(sdi,STDIN_FILENO);
        return;
    }
    dup2(sdo,STDOUT_FILENO);
    dup2(sdi,STDIN_FILENO);
    //外部命令
    execOuterCmd(cmd);
    
    //释放结构体空间
    // for(i = 0; i < cmd->argc; ++i){
    //     free(cmd->args[i]);
    // }
    // if (cmd->input!=NULL) free(cmd->input);
    // if (cmd->input!=NULL) free(cmd->output);
}

/*******************************************************
                     通配符匹配
********************************************************/
int pMatch(char* x, char* st, int x_begin, int st_begin)
{
    int len1 = strlen(x), lens = strlen(st);
    int i = x_begin,j = st_begin,k;
#ifdef ISDEBUG
    printf("matching %s and %s:\n",x + x_begin, st + st_begin);
#endif
    while (i < len1 && j < lens)
    {
        if (st[j]=='*')
            while (j + 1< lens&&st[j + 1]=='*') ++j;
#ifdef ISDEBUG
        printf("matching %c and %c: ",x[i],st[j]);
#endif
        if (st[j]!='*'&&st[j]!=x[i]) 
        {
#ifdef ISDEBUG
            printf("not match\n");
#endif
            return 0;
        }
        if (st[j]=='*'&&j==lens - 1) 
        {
#ifdef ISDEBUG
            printf("success.\n");
#endif
            return 1;
        }
        if (st[j]==x[i]&&st[j]!='*')
        {
#ifdef ISDEBUG
            printf("matched.\n");
#endif
            ++i;
            ++j;
            continue;
        }
        for (k = i;k < len1;++k)
        {
            if (x[k]==st[j + 1]) 
            {
                if (pMatch(x,st,k + 1, j + 2)) return 1;
            }
        }
#ifdef ISDEBUG
        printf("failed.\n");        
#endif
        return 0;
    }
    if (j < lens&&st[j]=='*')
        while (j + 1< lens&&st[j + 1]=='*') ++j;
    if (j==lens&&i==len1||j==lens - 1&&st[j]=='*') 
    {
#ifdef ISDEBUG
        printf("success.\n");
#endif
        return 1; 
    }
    else 
    {
#ifdef ISDEBUG
        printf("failed.\n");
#endif
        return 0;
    }
}
/*******************************************************
                     通配符扩展参数
********************************************************/
void extendArgs(SimpleCmd* head, char * s)
{
    DIR* dir;
    struct dirent *ptr;
    argC *ag;
    int flag=1, i, len;
    len = strlen(s);
    for (i = 0;i < len; ++i)
    {
        if (s[i]=='*')
        {
            flag = 0;
            break;
        }
    }
    if (flag || (dir = opendir(get_current_dir_name()))==NULL) 
    {
        ag = (argC*)malloc(sizeof(argC));
        ag->s = s;
        ag->next = head->ac;
        head->ac = ag;
        return;
    }
    flag = 1;
    for (ptr = readdir(dir);ptr!=NULL;ptr = readdir(dir))
    {
        if (s[0]!='.'&&(ptr->d_name[0]=='.')) continue;
        if (strcmp(head->cmd,"cd")==0&&ptr->d_type!=4) continue;
        if (pMatch(ptr->d_name,s,0,0))
        {
            flag = 0;
            ag = (argC*)malloc(sizeof(argC));
            ag->s = strdup(ptr->d_name);
            ag->next = head->ac;
            head->ac = ag;
        }
    }
    if (flag)
    {
        ag = (argC*)malloc(sizeof(argC));
        ag->s = s;
        ag->next = head->ac;
        head->ac = ag;
    }
}

/*******************************************************
                     调整&调试
********************************************************/
SimpleCmd* mEcho(SimpleCmd* head)
{
    SimpleCmd* p = head;
    SimpleCmd* tmp;
    int i=0,j;
    for (;p!=NULL;p=p->next)
    {
        //printf("%d: %d\n",++i,p->argc);
        //for (j=0;j<p->argc;++j) { printf("%s ",p->args[j]); }
        //printf("\nInput: %s\nOutput: %s\n",p->input,p->output);
        if (p->args[p->argc]!=NULL) 
        {
//            printf("overflow:\n");
//            printf("%s.\n",p->args[p->argc]);
            p->args[p->argc] = NULL;
        }
    }
    return NULL;
}

/*******************************************************
                     命令执行接口
********************************************************/
// void execute(){
//     SimpleCmd *cmd = handleSimpleCmdStr(0, strlen(inputBuff));
//     execSimpleCmd(cmd);
// }
//不再需要
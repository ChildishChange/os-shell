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
//#define DEBUG

#define DEBUG_PIPE
sigset_t WAIT,NONE;              //信号集用来描述信号的集合，linux所支持的所有信号可以全部或部分的出现在信号            
                          //集中，主要与信号阻塞相关函数配合使用.
int goon = 0, ignore = 0;       //用于设置signal信号量
char *envPath[10], cmdBuff[100];  //外部命令的存放路径及读取外部命令的缓冲空间
History history;                 //历史命令
Job *head = NULL;               //作业头指针
pid_t fgPid;                     //当前前台作业的进程号
#define MAX_PIPE 20
pid_t pipe_pid[MAX_PIPE];
int pipe_n,pipe_fg;
PipeJob *pipe_head=NULL,*now_pipe=NULL;
int stop_wait;
/*******************************************************
                  工具以及辅助方法
********************************************************/
void ctrl_Z();
void ctrl_C();
void rmJob();

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

void find_type(char *cmd_type){
    int i = 0;
    printf("type: ");
     if((cmd_type[0] == '/' || cmd_type[0] == '.') && access(cmd_type, F_OK) == 0){ //命令在当前目录
        strcpy(cmdBuff, cmd_type);
        printf("%s 命令在的当前目录下.\n",cmd_type);
        return ;
    }
   else if (strcmp(cmd_type,"alias")==0 ||
        strcmp(cmd_type,"bg")==0 ||
        strcmp(cmd_type,"bind")==0 ||
        strcmp(cmd_type,"break")==0 ||
        strcmp(cmd_type,"builtin")==0 ||
        strcmp(cmd_type,"caller")==0 ||
        strcmp(cmd_type,"cd")==0 ||
        strcmp(cmd_type,"command")==0 ||
        strcmp(cmd_type,"compgen")==0 ||
        strcmp(cmd_type,"complete")==0 ||
        strcmp(cmd_type,"compopt")==0 ||
        strcmp(cmd_type,"continue")==0 ||
        strcmp(cmd_type,"declare")==0 ||
        strcmp(cmd_type,"dirs")==0 ||
        strcmp(cmd_type,"disown")==0 ||
        strcmp(cmd_type,"echo")==0 ||
        strcmp(cmd_type,"enable")==0 ||
        strcmp(cmd_type,"eval")==0 ||
        strcmp(cmd_type,"exec")==0 ||
        strcmp(cmd_type,"exit")==0 ||
        strcmp(cmd_type,"export")==0 ||
        strcmp(cmd_type,"false")==0 ||
        strcmp(cmd_type,"fc")==0 ||
        strcmp(cmd_type,"fg")==0 ||
        strcmp(cmd_type,"getopts")==0 ||
        strcmp(cmd_type,"hash")==0 ||
        strcmp(cmd_type,"help")==0 ||
        strcmp(cmd_type,"history")==0 ||
        strcmp(cmd_type,"jobs")==0 ||
        strcmp(cmd_type,"kill")==0 ||
        strcmp(cmd_type,"let")==0 ||
        strcmp(cmd_type,"local")==0 ||
        strcmp(cmd_type,"logout")==0 ||
        strcmp(cmd_type,"mapfile")==0 ||
        strcmp(cmd_type,"popd")==0 ||
        strcmp(cmd_type,"printf")==0 ||
        strcmp(cmd_type,"pushd")==0 ||
        strcmp(cmd_type,"pwd")==0 ||
        strcmp(cmd_type,"read")==0 ||
        strcmp(cmd_type,"readarray")==0 ||
        strcmp(cmd_type,"readonly")==0 ||
        strcmp(cmd_type,"return")==0 ||
        strcmp(cmd_type,"set")==0 ||
        strcmp(cmd_type,"shift")==0 ||
        strcmp(cmd_type,"shopt")==0 ||
        strcmp(cmd_type,"source")==0 ||
        strcmp(cmd_type,"suspend")==0 ||
        strcmp(cmd_type,"test")==0 ||
        strcmp(cmd_type,"times")==0 ||
        strcmp(cmd_type,"trap")==0 ||
        strcmp(cmd_type,"true")==0 ||
        strcmp(cmd_type,"type")==0 ||
        strcmp(cmd_type,"typeset")==0 ||
        strcmp(cmd_type,"ulimit")==0 ||
        strcmp(cmd_type,"umask")==0 ||
        strcmp(cmd_type,"unalias")==0 ||
        strcmp(cmd_type,"unset")==0 ||
        strcmp(cmd_type,"wait")==0)
        {
        	printf("%s 命令是内建的.\n",cmd_type);
        	return;
    }


   else{  //查找ysh.conf文件中指定的目录，确定命令是否存在
        while(envPath[i] != NULL){ //查找路径已在初始化时设置在envPath[i]中
            strcpy(cmdBuff, envPath[i]);
            strcat(cmdBuff, cmd_type);

            if(access(cmdBuff, F_OK) == 0){ //命令文件被找到
                printf("%s 在下面目录中 %s.\n",cmd_type,cmdBuff);
                return ;
            }

            i++;
        }
    }
    printf("%s 没有找到该命令.\n",cmd_type);
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

void getenvironns(int n,char * s){
	int i = 0,j = 0,k = 0; 
	char c;   
	char buff[80];
	char * p; 
	while((c=s[i]) != '=') { 
		buff[i++] = c; 
	} 
	buff[i++] = '\0'; 
	if(strcmp(buff,"PATH") == 0) {
		while(s[i] != '\0'){ 
			if(s[i] == ':') {  
				buff[j++] = '/';   
				buff[j] = '\0'; 
				p = (char *)malloc(strlen(buff) + 1);    
				strcpy(p,buff);
				j = 0;       
				i++;
			} 
			else{ 
				buff[j] = s[i];      
				j++;
				i++;
			}
		}
	}
}



void add_new_pipe(pid_t pid){
    Job *job = (Job*)malloc(sizeof(Job));
    PipeJob *pipejob = (PipeJob*)malloc(sizeof(PipeJob)),*now;
    int i=0;
    while(inputBuff[i]){
        if (inputBuff[i] == '|') inputBuff[i] = '\0';
        i++;
    }

    job->pid = pid;
    strcpy(job->cmd, inputBuff);
    strcpy(job->state, RUNNING);
    job->next = NULL;

    pipejob->first = pipejob->now = job;
    pipejob->next_pipe = NULL;
    pipejob->n = pipe_n;
   if(pipe_head!=NULL){
        now = pipe_head;
        while(now->next_pipe!=NULL){
            now = now->next_pipe;
        }
        now->next_pipe = pipejob;
    }
 else{
        pipe_head = pipejob;
    }
    now_pipe = pipejob;
}

void insert_pipe(pid_t pid){
    Job *job = (Job*)malloc(sizeof(Job)),*now;

    job->pid = pid;
    strcpy(job->cmd, inputBuff);
    strcpy(job->state, RUNNING);
    job->next = NULL;

    now_pipe->now ->next = job;   //不断插入管道中的命令，由于第一个时候已经是创建新的所以这时候插入即可
    now_pipe->now = job;          // now_pipe->now当前的 
}

int functionpipe_debug(){
	int pfds[2];
	if ( pipe(pfds) == 0 ) {
		if ( fork() == 0 ) {
			close(1);
			dup2( pfds[1], 1 );
			close( pfds[0] );
			execlp( "ls", "ls", "-1", NULL );
		}
		else {
			close(0);
			dup2( pfds[0], 0 );
			close( pfds[1] );
			execlp( "wc", "wc", "-l", NULL );
		}
	}
	return 0;
}

int pipe_debug()
  {
            int thePipe[2], ret;
             char buf[100];
          const char *testbuf="a test string.";
    
           if ( pipe( thePipe ) == 0 ){
     
           if (fork() == 0) {
    
                 ret = read( thePipe[0], buf, 100 );
               buf[ret] = 0;
                printf( "Child read %s\n", buf );
  
               } else{
 
                ret = write( thePipe[1], testbuf, strlen(testbuf) );
                ret = wait( NULL );
    
              }
   
           }
}

void clean_pipe(){
    PipeJob *pipejob = pipe_head,*last;
    Job *job;
    last = NULL;
    while(pipejob){
        job = pipejob->first;
        while(job && pipejob->n){
            if (strcmp(job->state,FINISHED) != 0){
                if (waitpid(job->pid,NULL,WNOHANG) > 0){  //等待其他执行结束
                    strcpy(job->state, FINISHED);
                    pipejob->n --;
                }
            }
            job = job->next;
        }
      if(pipejob->n!=0){
            last = pipejob;
            pipejob = pipejob->next_pipe;
        }
        else{
            if (last == NULL)
                  pipe_head = pipejob->next_pipe;
            else 
                  last->next_pipe = pipejob->next_pipe;
            release_pipe(pipejob);
            pipejob = NULL;
            if (last) pipejob = last->next_pipe;
        }
       
    }
}


PipeJob* check_if_pipe(pid_t pid){
    PipeJob *pipejob = pipe_head;
    Job *job;
    if (pipe_head == NULL) return NULL;
    while(pipejob){
        job = pipejob->first;
        while(job){
            if (job->pid == pid) return pipejob;
            job = job->next;
        }
        pipejob = pipejob->next_pipe;
    }
    return NULL;
}

void release_pipe(PipeJob *pipe){
    Job *job,*last;
    job = pipe->first;
    last = job->next;
    rmJob(job->pid);
    while(job) {
        free(job);
        job = last;
        if (job!=NULL) last = job->next;
    }
    free(pipe);
}



void wait_pipe(PipeJob *pipejob,sigset_t t){
    Job *job;
    job = pipejob->first;

    while(pipejob->n && fgPid == job->pid){

        if (sigsuspend(&t) == -1){ ////sigsuspend用于在接收到某个信号之前，临时用mask替换进程的信号掩码，并暂停进程执行，直到收到信号为止。
        }else break;
    }
}



int hash_pipedebug()
{
    int i, j, prime, temp, n=0;
    int num[5000]={0}, hash[5000][2]={{0}, {0}};
    int position=0;
    char op;
    do{
        scanf("%d", &num[n]);
        n++;
    }
    while ((op = getchar()) != '#');
    for (i=0;i<n;i++){
        for (j=i;j<n;j++){
            if (num[i] > num[j]){
                temp = num[i];
                num[i] = num[j];
                num[j] = temp;
            }
        }
    }
    for (i=n;i>=2;i--){
        for (j=2;j<=i/2;j++){
            if (i%j == 0){
                break;
            }
        }
        if ((j == (i/2)+1) && (i%j != 0)){
            break;
        }
    }
    prime = i;
    for (i=0;i<=n;i++){
        position = num[i] % prime;
        hash[position][0] = num[i];
        hash[position][1]++;
        if (num[i+1] != num[i]){
            printf("%d=%d\n", hash[position][0], hash[position][1]);
            hash[position][0] = 0;
            hash[position][1] = 0;
        }
    }
    return 0;
}


/*进程执行到sigsuspend时，sigsuspend并不会立刻返回，进程处于TASK_INTERRUPTIBLE状态并立刻放弃CPU，等待UNBLOCK（mask之外的）信号的唤醒。进程在接收到UNBLOCK（mask之外）信号后，调用处理函数，然后把现在的信号集还原为原来的，sigsuspend返回，进程恢复执行。sigsuspend返回后将恢复调用之前的的信号掩码。信号处理函数完成后，进程将继续执行。该系统调用始终返回-1，并将errno设置为EINTR.*/
/*移除一个作业*/
void rmJob(int pid){
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
	//sleep(1);
}

//


void HeapAdjust(int array[],int i,int nLength)
{
    int nChild;
    int nTemp;
    for(;2*i+1<nLength;i=nChild)
    {
        nChild=2*i+1;
        if(nChild<nLength-1 && array[nChild+1]>array[nChild])
            ++nChild;
        if(array[i]<array[nChild]){
            nTemp=array[i];
            array[i]=array[nChild];
            array[nChild]=nTemp;
        }
        else
            break;
    }
}

void CHLD(int sig, siginfo_t *sip, void* noused){
    pid_t pid;
    Job *now = NULL, *last = NULL;
    PipeJob *pipejob;
    int i;
    int pipe_flg;

    if (sip->si_status == CLD_CONTINUED || sip->si_status == SIGCONT ||
        sip->si_status == SIGTTOU || sip->si_status == SIGTTIN) return;

    // Ctrl z = SIGTSTP

    //if (sip->si_status == SIGTSTP) return;
    if (sip->si_status == SIGTSTP) {
        ctrl_Z();
        return;
    }
   else{
        if (sip->si_status == CLD_KILLED){
            ctrl_C();
        }
    }
    if (ignore){
        ignore = 0;
        return;
    }


    pid = sip->si_pid;

    //处理管道的进程组问题
    clean_pipe();
    pipejob = check_if_pipe(pid);
    if (pipejob !=NULL ) 
          return ;
    waitpid(pid, NULL, 0);
    tcsetpgrp(0,getpid()); //恢复主进程到前台
    rmJob(pid);
    if (pid != fgPid) 
         tcsetpgrp(0,fgPid);
}

//组合键ctrl_c
void ctrl_C(){
  Job *now = NULL;
  if (fgPid == 0){
    return;
  }
  ignore = 0;
  tcsetpgrp(0,getpid());
  now = head;
	while(now != NULL && now->pid != fgPid)
		now = now->next;

    if(now == NULL){ //未找到前台作业，则根据fgPid添加前台作业
        now = addJob(fgPid);
    }

	//修改前台作业的状态及相应的命令格式，并打印提示信息
    strcpy(now->state, KILLED);
    now->cmd[strlen(now->cmd)] = '&';
    now->cmd[strlen(now->cmd) + 1] = '\0';
    printf("[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);

	//发送SIGSTOP信号给正在前台运作的工作，将其退出
    kill(fgPid, SIGKILL);
    fgPid = 0;
   //printf("yi ctrl_c\n");
}

/*组合键命令ctrl+z*/
void ctrl_Z(){
    Job *now = NULL;
    int i;
    tcsetpgrp(0,getpid());
    if(fgPid == 0){ //前台没有作业则直接返回
        return;
    }

    //SIGCHLD信号产生自ctrl+z

    ignore = 1;
    pipe_fg = 0;
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
    stop_wait = 1;   // 不调用 wait_pipe
	//发送SIGSTOP信号给正在前台运作的工作，将其停止
    kill(fgPid, SIGSTOP);
    fgPid = 0;
    // printf("yi ctrl_z\n");
}

/*void getnam_debug(char* name)  { 
int i;  
for(i=0;i<100;++i)  {   
  switch(SimpleCmd[i])     {   
     case '>':  
    case '<':  
   case '|':    
   case '&':  
   case ' ':  
  case '\t': 
   case '\n':   
      *name='\0';       
   }  
}  
*name='\0'; 
}  */

void Sig_void(){
}

/*fg命令*/
void fg_exec(int pid){
    Job *now = NULL;
    PipeJob *pipejob;
	int i,ret;

    //SIGCHLD信号产生自此函数
    ignore = 0;

	//根据pid查找作业
    now = head;
	while(now != NULL && now->pid != pid)
		now = now->next;

    if(now == NULL){ //未找到作业
        printf("pid为%d 的作业不存在！\n", pid);
        return;
    }

    //pipe
    pipejob = check_if_pipe(now->pid);
    if (pipejob!=NULL) 
        pipe_fg = 1;

    //记录前台作业的pid，修改对应作业状态
    fgPid = now->pid;
    strcpy(now->state, RUNNING);

    //signal(SIGTSTP, Sig_void); //设置signal信号，为下一次按下组合键Ctrl+Z做准备
    i = strlen(now->cmd) - 1;
    while(i >= 0 && now->cmd[i] != '&')
		i--;
    now->cmd[i] = '\0';

    printf("%s\n", now->cmd);

    killpg(now->pid, SIGCONT); //向对象作业发送SIGCONT信号，使其运行
    if (tcsetpgrp(0,now->pid) < 0){
        perror("将前台进程组ID设置为PID失败\n");
    }

    if (pipejob!=NULL){
        if (!stop_wait) wait_pipe(pipejob,WAIT);
        stop_wait = 0;
    }
    else {
        sigsuspend(&WAIT);
    }

    tcsetpgrp(0,getpid());
}

/*bg命令*/
void bg_exec(int pid){
    Job *now = NULL;
    int i;
    //SIGCHLD信号产生自此函数
    ignore = 1;

    //检测是否为pipe
    for(i=1;i<=pipe_n;i++){
        if (pipe_pid[i] == pid) {
            break;
        }
    }
    if (i<=pipe_n) 
       pipe_fg = 0;


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
   tcsetpgrp(0,getpid());
    killpg(now->pid, SIGCONT); //向对象作业发送SIGCONT信号，使其运行
   // waitpid(now->pid,NULL,0);//不在意任何状态返回值
/*
    pid>0时，只等待进程ID等于pid的子进程，不管其它已经有多少子进程运行结束退出了，只要指定的子进程还没有结束，waitpid就会一直等下去。
    pid=-1时，等待任何一个子进程退出，没有任何限制，此时waitpid和wait的作用一模一样。
    pid=0时，等待同一个进程组中的任何子进程，如果子进程已经加入了别的进程组，waitpid不会对它做任何理睬。
    pid<-1时，等待一个指定进程组中的任何子进程，这个进程组的ID等于pid的绝对值。*/
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


int judge_debug(int s[], int left, int right){
    int i, j;
    int root = s[right];
    if(s == NULL || left < 0 || right < left){
        return 0;
    }
    int len = right - left + 1;
    for(i=left; i<left+len-1; i++){
        if(s[i] > root){
            break;
        }
    }
    for (j=i;j<left+len-1;j++){
        if (s[j] < root){
            return 0;
        }
    }
    if (i>0){
        judge_debug(s, left, i-1);
    }
    if (i<left+len-1){
        judge_debug(s, i, right-1);
    }
    return 1;
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


/*******************************************************
                      命令解析
********************************************************/
SimpleCmd* handleSimpleCmdStr(int begin, int end){
    int i, j, k;
    int fileFinished; //记录命令是否解析完毕
    char c, buff[100][40], inputFile[30], outputFile[30], *temp = NULL;
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
    while(i < end && (inputBuff[i] == ' ' || inputBuff[i] == '\t' || inputBuff[i] == '\n')){
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

    return cmd;
}

/*******************************************************
                      命令执行
********************************************************/

/*执行命令*/
void execSimpleCmd(SimpleCmd *cmd, int dup_flg, pid_t in_pid, pid_t *out_pid ,int in_filedes, int out_filedes, int inputBuff_start){
    int i, pid;
    char *temp;
    Job *now = NULL;

    if(strcmp(cmd->args[0], "exit") == 0) { //exit命令
        exit(0);
    }
else if (strcmp(cmd->args[0], "history") == 0) { //history命令
        if(history.end == -1){
            printf("尚未执行任何命令\n");
            return;
        }
        i = history.start;
        do {
            printf("%s\n", history.cmds[i]);
            i = (i + 1)%HISTORY_LEN;
        } while(i != (history.end + 1)%HISTORY_LEN);
    }
else if (strcmp(cmd->args[0], "jobs") == 0) { //jobs命令
        if(head == NULL){
            printf("尚无任何作业\n");
        } else {
            printf("index\tpid\tstate\t\tcommand\n");
            for(i = 1, now = head; now != NULL; now = now->next, i++){
                printf("%d\t%d\t%s\t\t%s\n", i, now->pid, now->state, now->cmd);
            }
        }
    }

else if (strcmp(cmd->args[0], "fg") == 0) { //fg命令
        temp = cmd->args[1];
        if(temp != NULL && temp[0] == '%'){
            pid = str2Pid(temp, 1, strlen(temp));
            if(pid != -1){
                fg_exec(pid);
            }
        }else{
            printf("fg参数不合法，fg命令正确格式为：fg %%<int>\n");
        }
    }


else if (strcmp(cmd->args[0], "bg") == 0) { //bg命令
        temp = cmd->args[1];
        if(temp != NULL && temp[0] == '%'){
            pid = str2Pid(temp, 1, strlen(temp));

            if(pid != -1){
                bg_exec(pid);
            }
        }
		else{
            printf("bg参数不合法，fg命令正确格式为：bg %%<int>\n");
        }
    }
else if (strcmp(cmd->args[0],"type") == 0) { //type  打印命令的类型
        find_type(cmd->args[1]);
    }
 
else if (strcmp(cmd->args[0], "cd") == 0) { //cd命令
        temp = cmd->args[1];
        if(temp != NULL){
            if(chdir(temp) < 0){
                printf("cd; %s 错误的文件名或文件夹名！\n", temp);
            }
        }
    }

else if (strcmp(cmd->args[0],"echo") == 0) { //echo
         printf("输入的内容为：\n");
        for (i=1;cmd->args[i];i++) 
             printf("%s\n",cmd->args[i]);
        printf("\n");
    } 
else if (strcmp(cmd->args[0],"reset") == 0) { // reset  这个命令将完全刷新终端屏幕，
         printf("将要清楚屏幕\n");                    //之前的终端输入操作信息将都会被清空，这样虽然比较清爽，但整个命令过程速度有点慢，使用较少。
         sleep(1); 
         printf("\033\143");
        printf("已经清除屏幕！\n");
    } 
else if (strcmp(cmd->args[0],"clear") == 0) { // clear  这个命令将会刷新屏幕，本质上只是让终端显示页向后翻了一页，
                                               //如果向上滚动屏幕还可以看到之前的操作信息。一般都会用这个命令。
        printf("将要清楚屏幕\n");
         sleep(1);
        printf("\033[H\033[J");
        printf("已经清除屏幕！\n");
    } 
else if (strcmp(cmd->args[0],"pwd") == 0) { // pwd
        printf("%s\n",get_current_dir_name());
    }
else{ //外部命令
        execOuterCmd(cmd, dup_flg, in_pid, out_pid , in_filedes , out_filedes, inputBuff_start);
    }

    //释放结构体空间
    for(i = 0; cmd->args[i] != NULL; i++){
        free(cmd->args[i]);
        free(cmd->input);
        free(cmd->output);
    }
}

/*执行外部命令*/
void execOuterCmd(SimpleCmd *cmd, int dup_flg, pid_t in_pid, pid_t *out_pid ,int in_filedes, int out_filedes, int inputBuff_start){
    pid_t pid;
    int pipeIn, pipeOut;

    if(exists(cmd->args[0])){ //命令存在

        if((pid = fork()) < 0){
            perror("fork failed");
            return;
        }

        if(pid == 0){ //子进程
            if(cmd->output != NULL){ //存在输出重定向
                if((pipeOut = open(cmd->output, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1){
                    printf("不能打开文件 %s！\n", cmd->output);
                    return ;
                }
                if(dup2(pipeOut, 1) == -1){   //标准输出
                    printf("重定向标准输出错误！\n");
                    return;
                }
            }
           else if ((dup_flg&2)==2){   //10对应10 11才会得到2的结果，即除了最后一个管道
                if(dup2(out_filedes, 1) == -1){
                    printf("标准输出管道错误！\n");
                    return;
                }else{
                    close(out_filedes);
                }
            }
           if(cmd->input != NULL){ //存在输入重定向，输入重定向在管道中只可能为第一个管道命令
                if((pipeIn = open(cmd->input, O_RDONLY, S_IRUSR|S_IWUSR)) == -1){
                    printf("不能打开文件 %s！\n", cmd->input);
                    return;
                    }
                if(dup2(pipeIn, 0) == -1){//标准输入
                    printf("重定向标准输入错误！\n");
                    return;
                }
            }
           else if ((dup_flg&1)==1){        //dup_flg为10（即第一个管道时为假，其他为真）
                if(dup2(in_filedes,0) == -1){
                    printf("标准输入管道错误！\n");
                    return;
                }else{
                    close(in_filedes);      
                }
            }

          
            /*

            if (dup_flg == 2){
                close(in_filedes);

            }*/
            if(cmd->isBack){ //若是后台运行命令，等待父进程增加作业
                signal(SIGUSR1, setGoon); //收到信号，setGoon函数将goon置1，以跳出下面的循环
                kill(getppid(),SIGUSR1);  //成功执行时，返回0。失败返回-1  int kill(pid_t pid, int sig);

                               //函数说明：kill()可以用来送参数sig 指定的信号给参数pid 指定的进程，即通知父进程已经完成
                while(goon == 0) ; //等待父进程SIGUSR1信号，表示作业已加到链表中
                goon = 0; //置0，为下一命令做准备

                printf("[%d]\t%s\t\t%s\n", getpid(), RUNNING, inputBuff);
                //kill(getppid(), SIGCHLD);
            }

            justArgs(cmd->args[0]);  //去掉最后一个/之前的符号
            if(execv(cmdBuff, cmd->args) < 0){ //执行命令
                printf("execv failed!\n");
                return;
            }

        }
		else{ //父进程
            if (dup_flg){         //存在管道命令
                if (dup_flg == 2)
                      add_new_pipe(pid);//第一个管道命令应该新建一个管道指针
                else 
                      insert_pipe(pid);//第二个管道以后就可以开始插入之前的管道
            }
            if (dup_flg==0 || dup_flg==2){   //第一个管道命令或者没有管道命令
                if (setpgid(pid,pid) < 0){   //将PID进程组的进程组号设置为PID
                    printf("setpgid faild!\n");
                }else{
                    *out_pid = pid;
                }
            }
           else{
                if (setpgid(pid, in_pid) < 0){
                    printf("setpgid faild!\n");
                }else{
                    *out_pid = pid;
                }
            }
           if(!cmd->isBack){ //不是后台命令
                if (dup_flg==0 || dup_flg==2) {
                    fgPid = pid;
                    tcsetpgrp(0,pid);
                }
                /*else{
                    tcsetpgrp(0,in_pid);
                }*/
                //tcsetpgrp(1,pid);
                if (!dup_flg) {
                    waitpid(pid, NULL, 0);
                    tcsetpgrp(0,getpid());
                }
            }
            else{ //后台命令
                fgPid = 0; //pid置0，为下一命令做准备
                //先等待子进程发射 SIGUSR1
                signal(SIGUSR1, setGoon);
                while(goon == 0) ;
                if (dup_flg == 0 || dup_flg == 2)
                        addJob(pid); //第一个管道命令或者不是管道命令时增加新的作业
                kill(pid, SIGUSR1); //子进程发信号，表示作业已加入


                //等待子进程输出
                //signal(SIGUSR1, setGoon);
                //while(goon == 0) ;
                goon = 0;
            }
        
		}
    }else{ //命令不存在
        printf("找不到命令 %s\n", inputBuff + inputBuff_start);
    }


}


void main_pipe_test()
{
    int fd[2], res;
    pid_t pid;

    res = pipe(fd);
    if(res == -1) {
        perror("Pipe error!");
        exit(1);
    }
    pid = fork();
    if(pid > 0) {
        close(fd[1]);
        dup2(fd[0], 0);
        execl("/usr/bin/sort", "sort", (char *)0);
        wait(NULL);
        exit(0);
    } else if(pid == 0) {
        dup2(fd[1], 1);
        close(fd[0]);
        execl("/bin/cat", "cat", "file", NULL);
        exit(0);
    } else {
        perror("Fork error!");
        exit(2);
    }
}


/*******************************************************
                     命令执行接口
********************************************************/
void execute(){
    pid_t pid;
    SimpleCmd *cmd = handleSimpleCmdStr(0, strlen(inputBuff));
    stop_wait = 0;
    execSimpleCmd(cmd, 0, 0, &pid, 0, 0, 0);
}


void execute_pipe(){
    int n=0,i=0,r[MAX_PIPE],len;
    SimpleCmd *cmd[MAX_PIPE];
    int pipe_fd[MAX_PIPE][2];
    pid_t *pid;
    pid = pipe_pid;


    stop_wait = 0;
    len=strlen(inputBuff);

    r[0] = 0;
    for(i=0;i<len;i++)
        if (inputBuff[i] == '|'){
            r[++n] = i;
            inputBuff[i] = '\0';
        }
    r[++n] = len;

    cmd[1] = handleSimpleCmdStr(0, r[1]);
    for(i=2;i<n;i++) cmd[i] = handleSimpleCmdStr(r[i-1] + 1, r[i]);
    cmd[n] = handleSimpleCmdStr(r[n-1] + 1, r[n]);

    // 处理全局变量
    pipe_n = n;  //pipe_n就是管道的数目

    if (pipe(pipe_fd[1]) < 0){    //pipe_fd[n][0]管道输入描述符.1为输出描述符
        perror("管道创建失败\n");
    }
    fgPid = pid[1];
    pipe_fg = 1;           //前台有程序运行的标志
    execSimpleCmd(cmd[1], 2, 0, &pid[1],  pipe_fd[1][0], pipe_fd[1][1] , 0);
    close(pipe_fd[1][1]);

    for(i=2;i<n;i++){
        if (pipe(pipe_fd[i]) < 0){
            perror("管道创建失败\n");
        }

        execSimpleCmd(cmd[i], 3, fgPid, &pid[i], pipe_fd[i-1][0] , pipe_fd[i][1], r[i-1] + 1);

        close(pipe_fd[i-1][0]);
        close(pipe_fd[i][1]);
    }
//传到外部指令中dup_flg:
               // 0 : 不重定向     没有管道命令
                //1 : 输入管道     第n个管道命令
                //2 : 输出管道   第一个管道命令
                //3 : 同时有输出输出管道   1-n-1个管道命令
    execSimpleCmd(cmd[n], 1, fgPid, &pid[n], pipe_fd[n-1][0] , 0 , r[n-1] + 1 );
    close(pipe_fd[n-1][0]);
    for(i=1;i<n;i++) inputBuff[r[i]] = '|';

    if (!stop_wait) wait_pipe(now_pipe,NONE);
    stop_wait = 0;
    tcsetpgrp(0,getpid());
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

    pipe_n  = 0;
    pipe_fg = 0;
    stop_wait = 0;
    //注册信号

    struct sigaction action;
    action.sa_sigaction = CHLD;
    sigfillset(&action.sa_mask);
    action.sa_flags = SA_SIGINFO;
    sigaction(SIGCHLD, &action, NULL);
    signal(SIGTTOU , Sig_void);
    signal(SIGTTIN , Sig_void);
    signal(SIGUSR1, setGoon);
    //signal(SIGTSTP, ctrl_Z);
    //signal(SIGINT , ctrl_C);
    signal(SIGTSTP, Sig_void);
    signal(SIGINT , Sig_void);
   

    sigemptyset(&WAIT);
    sigemptyset(&NONE);      //清空信号集合set
    sigaddset(&WAIT, CLD_CONTINUED);   //往set中添加信号signum 
    sigaddset(&WAIT, SIGCONT);
   
}


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
int blocked = 1;       //用于设置signal信号量
int fildes[2];	//用于存放管道文件描述符
char *envPath[10], cmdBuff[40];  //外部命令的存放路径及读取外部命令的缓冲空间
History history;                 //历史命令
Job *head = NULL;                //作业头指针
pid_t fgPid, chainGid;                     //当前前台作业的进程号，shell组号，管道链进程组号

/****************************************************************
                  测试用函数
****************************************************************/
extern void a1();
extern void a2();
extern void a3();
extern void pCmds();
void perr(){
    printf("errno:%d\n", errno);
}
void pPipe(SimpleCmd *cmd, int pi, int po){
    printf("cmd:%s\tpflag:%d\n", cmd->args[0], cmd->pflag);
    printf("input:%s\toutput:%s\n", cmd->input, cmd->output);
    printf("ipipe:%d\topipe:%d\n", pi, po);
    printf("fildes0:%d\tfildes1:%d\n", fildes[0], fildes[1]);
}

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
	char chs[10];
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

/*释放空间*/
void release(){
    int i;
    for(i = 0; strlen(envPath[i]) > 0; i++){
        free(envPath[i]);
    }
}

/*预处理管道链*/
void pretreat(SimpleCmd **cmds, int n){
    int i;
    if (n <= 1){
        cmds[0]->pflag = 0;
        return;
    }
    for (i = 0; i <= n - 1; i++){
        if (i == 0){
            if (cmds[i]->output != NULL){
                cmds[i]->pflag = 0;
            } else {
                cmds[i]->pflag = 2;
            }
        } else if (i == n - 1){
            if (cmds[i]->input != NULL){
                cmds[i]->pflag = 0;
            } else {
                cmds[i]->pflag = 1;
            }
        } else {
            if (cmds[i]->input != NULL && cmds[i]->output != NULL){
                cmds[i]->pflag = 0;
            } else if (cmds[i]->input != NULL){
                cmds[i]->pflag = 2;
            } else if (cmds[i]->output != NULL){
                cmds[i]->pflag = 1;
            } else {
                cmds[i]->pflag = 3;
            }
        }
    }
}

void buildPipe(){
    pipe(fildes);
    fcntl(fildes[0], F_SETFL, O_NONBLOCK); 
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
    if (sip->si_status == SIGTSTP ||sip->si_status == SIGSTOP
	|| sip->si_status == SIGCONT 
	|| sip->si_status == SIGTTIN || sip->si_status == SIGTTOU){
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
    } else {
        last->next = now->next;
    }
    kill(now->pid, SIGKILL);
    free(now);
}

void unblock(){
    blocked = 0;
}

void ctrl_Z(){
    Job *now = NULL;
    if(fgPid == 0) return;

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
        printf("\n[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);
        //发送SIGSTOP信号给正在前台运作的工作，将其停止
        kill(fgPid, SIGSTOP);
        fgPid = 0;
}

/*组合键ctrl+c*/
void ctrl_C(){
    if (fgPid == 0) return;
    else kill(fgPid, SIGKILL);
}

/*fg命令*/
void fg_exec(int pid){    
    Job *now = NULL; 
    int i;
    

    //根据pid查找作业
    now = head;
    while(now != NULL && now->pid != pid){
        now = now->next;
    }
    
    if(now == NULL){ //未找到作业
        printf("pid为%d 的作业不存在！\n", pid);
        return;
    }

    //记录前台作业的pid，修改对应作业状态
    fgPid = now->pid;
    strcpy(now->state, RUNNING);

    i = strlen(now->cmd) - 1;
    while(i >= 0 && now->cmd[i] != '&'){
        i--;
    }
    now->cmd[i] = '\0';

    kill(now->pid, SIGCONT); //向对象作业发送SIGCONT信号，使其运行
    sleep(1);
    waitpid(fgPid, NULL, WUNTRACED); //父进程等待前台进程的运行
}

/*bg命令*/
void bg_exec(int pid){
    Job *now = NULL;

    //根据pid查找作业
    now = head;
    while(now != NULL && now->pid != pid){
        now = now->next;
    }
    if(now == NULL){ //未找到作业
        printf("pid为%d 的作业不存在！\n", pid);
        return;
    }
    
    strcpy(now->state, RUNNING); //修改对象作业的状态
    printf("[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);
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
    int fd, len, pid;
    char c, buf[80];

    //打开查找路径文件atea-sh.conf
    if((fd = open("atea-sh.conf", O_RDONLY, 660)) == -1){
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
    close(fd);
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
    signal(SIGUSR1, unblock);
    signal(SIGTTOU, SIG_IGN);
}

/*******************************************************
                      命令执行
********************************************************/
/*重定向*/
void ioAlloc(i, o){
    if (dup2(i, 0) < 0){
        printf("error in dup2 input");    
    }
    if (dup2(o, 1) < 0){
        printf("error in dup2 output");
    }
}

void ioDirec(SimpleCmd *cmd, pid_t pid){
    int i = 0, o = 1;
    char buf[PIPEBUFSIZE];
    if(cmd->pflag == 0){ //不用管道
        if(cmd->input != NULL && (i = open(cmd->input, O_RDONLY, S_IRUSR|S_IWUSR)) == -1){
            printf("不能打开文件 %s！\n", cmd->input);
            return;
        }
        if(cmd->output != NULL && (o = open(cmd->output, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1){
            printf("不能打开文件 %s！\n", cmd->output);
            return ;
        }
        if (pid == 0){
            //close(fildes[0]);
            //close(fildes[1]);
            while (read(fildes[1], buf, PIPEBUFSIZE) > 0);
            pPipe(cmd, i, o);
            ioAlloc(i, o);
        }
    } else if (cmd->pflag == 1){ //只用管道读         
        if(cmd->output != NULL && (o = open(cmd->output, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1){
            printf("不能打开文件 %s！\n", cmd->output);
            return ;
        }
        i = fildes[0];
        if (pid == 0){
            //close(fildes[1]);
            pPipe(cmd, i, o);
            ioAlloc(i, o);
        }
    } else if (cmd->pflag == 2){ //只用管道写
        if(cmd->input != NULL && (i = open(cmd->input, O_RDONLY, S_IRUSR|S_IWUSR)) == -1){
            printf("不能打开文件 %s！\n", cmd->input);
            return;
        }
        o = fildes[1];
        if (pid == 0){
            //close(fildes[0]);
            while (read(fildes[1], buf, PIPEBUFSIZE) > 0);
            pPipe(cmd, i, o);
            ioAlloc(i, o);
        }
    } else { //不存在重定向
        i = fildes[0];
        o = fildes[1];
        if (pid == 0){
            pPipe(cmd, i, o);
            ioAlloc(i, o);
        }        
    }
}


/*执行外部命令*/
void execOuterCmd(SimpleCmd *cmd){
    pid_t pid;
    int i;
    if(exists(cmd->args[0])){ //命令存在
        if((pid = fork()) < 0){
            perror("fork failed");
            return;
        }

        ioDirec(cmd, pid);
        if(pid == 0){ //子进程
            if (cmd->pflag == 0 || cmd->pflag == 1){
                setpgid(0, 0);
            } else {
                setpgid(0, chainGid);
            }
            kill(getppid(), SIGUSR1);
            if(cmd->isBack){ //若是后台运行命令，等待父进程增加作业
                while(blocked); //等待父进程将作业添加到链表中
                blocked = 1;
                printf("[%d]\t%s\t\t%s\n", getpid(), RUNNING, inputBuff);
                kill(getppid(), SIGUSR1); //信息已输出
            }
            justArgs(cmd->args[0]);
            if(execv(cmdBuff, cmd->args) < 0){ //执行命令
                printf("execv failed!\n");
                return;
            }
        } else { //父进程
            while (blocked);
            blocked = 1;
            if (cmd->pflag == 0 || cmd->pflag == 1) chainGid = getpgid(pid);
            if (cmd ->isBack){ //后台命令
                fgPid = 0; //pid置0，为下一命令做准备
                addJob(pid); //增加新的作业
                kill(pid, SIGUSR1); //通知子进程作业已加入
                while (blocked); //等待子进程输出
                blocked = 1;
            } else { //非后台命令
                fgPid = pid;
                tcsetpgrp(0, chainGid);
                waitpid(pid, NULL, WUNTRACED);
                tcsetpgrp(0, getpgrp());
            }
        }
    } else { //命令不存在
        printf("找不到命令%s\n", inputBuff);
    }
}

/*执行命令*/

void bi_exit(SimpleCmd *cmd){
    //cmd->pflag = 0;
    //ioReAlloc(cmd, 0);
    exit(0);
}

void bi_history(SimpleCmd *cmd){
    int i;
    //if (cmd->pflag != 0) cmd->pflag = 1;
    //ioReAlloc(cmd, 0);
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

void bi_jobs(SimpleCmd *cmd){
    int i;
    //if (cmd->pflag != 0) cmd->pflag = 1;
    //ioReAlloc(cmd, 0);
    Job *now = NULL;
    if(head == NULL){
        printf("尚无任何作业\n");
    } else {
        printf("index\tpid\tstate\t\tcommand\n");
        for(i = 1, now = head; now != NULL; now = now->next, i++){
            printf("%d\t%d\t%s\t\t%s\n", i, now->pid, now->state, now->cmd);
        }
    }
}

void bi_cd(SimpleCmd *cmd){
    char *temp;
    //cmd->pflag = 0;
    //ioReAlloc(cmd, 0);
    temp = cmd->args[1];
    if(temp != NULL){
        if(chdir(temp) < 0){
            printf("cd; %s 错误的文件名或文件夹名！\n", temp);
        }
    }
}

void bi_fg(SimpleCmd *cmd){
    int pid;
    char *temp;
    //cmd->pflag = 0;
    //ioReAlloc(cmd, 0);
    temp = cmd->args[1];
    if(temp != NULL && temp[0] == '%'){
        pid = str2Pid(temp, 1, strlen(temp));
        if(pid != -1){
            fg_exec(pid);
        }
    }else{
        printf("fg; 参数不合法，正确格式为：fg %<int>\n");
    }
}

void bi_bg(SimpleCmd *cmd){
    int pid;
    char *temp;
    //cmd->pflag = 0;
    //ioReAlloc(cmd, 0);
    temp = cmd->args[1];
    if(temp != NULL && temp[0] == '%'){
        pid = str2Pid(temp, 1, strlen(temp));
        if(pid != -1){
            bg_exec(pid);
        }
    } else {
        printf("bg; 参数不合法，正确格式为：bg %<int>\n");
    }
}

void bi_echo(SimpleCmd *cmd){
    int i;
    //if (cmd->pflag != 0) cmd->pflag = 1;
    //ioReAlloc(cmd, 0);
    for (i = 1; i <= cmd->argc - 1; i++){
        printf("%s ", cmd->args[i]);
    }
    printf("\n");
}

void bi_exec(SimpleCmd *cmd){
    int i;
    void execSimpleCmd(SimpleCmd *cmd);
    //cmd->pflag = 0;
    //ioReAlloc(cmd, 0);
    cmd->argc--;
    free(cmd->args[0]);
    for (i = 0; i <= cmd->argc - 1; i++) cmd->args[i] = cmd->args[i + 1];
    cmd->args[i] = NULL;
    free(cmd->input);
    cmd->input = NULL;
    free(cmd->output);
    cmd->output = NULL;
    execSimpleCmd(cmd);
}

void execSimpleCmd(SimpleCmd *cmd){
    if(strcmp(cmd->args[0], "exit") == 0) { //exit命令
        bi_exit(cmd);
    } else if (strcmp(cmd->args[0], "history") == 0) {//history命令
        bi_history(cmd);
    } else if (strcmp(cmd->args[0], "jobs") == 0) { //jobs命令
        bi_jobs(cmd);
    } else if (strcmp(cmd->args[0], "cd") == 0) { //cd命令
        bi_cd(cmd);
    } else if (strcmp(cmd->args[0], "fg") == 0) { //fg命令
        bi_fg(cmd);
    } else if (strcmp(cmd->args[0], "bg") == 0) { //bg命令
        bi_bg(cmd);
    } else if (strcmp(cmd->args[0], "echo") == 0){ //echo命令
        bi_echo(cmd);
    } else if (strcmp(cmd->args[0], "exec") == 0){
        bi_exec(cmd);
    } else { //外部命令
        execOuterCmd(cmd);
    }
}

/******************************************************
                     命令执行接口
********************************************************/
void executeCmds(SimpleCmd **cmds, int n){
    int i;
    buildPipe();
    pretreat(cmds, n);
    for (i = 0; i <= n - 1; i++){
        execSimpleCmd(cmds[i]);
    }
    close(fildes[0]);
    close(fildes[1]);
} 

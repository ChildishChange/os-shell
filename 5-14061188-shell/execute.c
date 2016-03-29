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
int blocked = 1;       //��������signal�ź���
int fildes[2];	//���ڴ�Źܵ��ļ�������
char *envPath[10], cmdBuff[40];  //�ⲿ����Ĵ��·������ȡ�ⲿ����Ļ���ռ�
History history;                 //��ʷ����
Job *head = NULL;                //��ҵͷָ��
pid_t fgPid, chainGid;                     //��ǰǰ̨��ҵ�Ľ��̺ţ�shell��ţ��ܵ����������

/****************************************************************
                  �����ú���
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
                  �����Լ���������
********************************************************/
/*�ж������Ƿ����*/
int exists(char *cmdFile){
    int i = 0;
    if((cmdFile[0] == '/' || cmdFile[0] == '.') && access(cmdFile, F_OK) == 0){ //�����ڵ�ǰĿ¼
        strcpy(cmdBuff, cmdFile);
        return 1;
    }else{  //����ysh.conf�ļ���ָ����Ŀ¼��ȷ�������Ƿ����
        while(envPath[i] != NULL){ //����·�����ڳ�ʼ��ʱ������envPath[i]��
            strcpy(cmdBuff, envPath[i]);
            strcat(cmdBuff, cmdFile);

            if(access(cmdBuff, F_OK) == 0){ //�����ļ����ҵ�
                return 1;
            }
            
            i++;
        }
    }
    
    return 0; 
}

/*���ַ���ת��Ϊ���͵�Pid*/
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

/*���������ⲿ����ĸ�ʽ*/
void justArgs(char *str){
    int i, j, len;
    len = strlen(str);
    
    for(i = 0, j = -1; i < len; i++){
        if(str[i] == '/'){
            j = i;
        }
    }

    if(j != -1){ //�ҵ�����'/'
        for(i = 0, j++; j < len; i++, j++){
            str[i] = str[j];
        }
        str[i] = '\0';
    }
}

/*�ͷſռ�*/
void release(){
    int i;
    for(i = 0; strlen(envPath[i]) > 0; i++){
        free(envPath[i]);
    }
}

/*Ԥ����ܵ���*/
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
                  �ź��Լ�jobs���
********************************************************/
/*����µ���ҵ*/
Job* addJob(pid_t pid){
    Job *now = NULL, *last = NULL, *job = (Job*)malloc(sizeof(Job));
    
	//��ʼ���µ�job
    job->pid = pid;
    strcpy(job->cmd, inputBuff);
    strcpy(job->state, RUNNING);
    job->next = NULL;
    
    if(head == NULL){ //���ǵ�һ��job��������Ϊͷָ��
        head = job;
    }else{ //���򣬸���pid���µ�job���뵽����ĺ���λ��
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

/*�Ƴ�һ����ҵ*/
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
    if(now == NULL){ //��ҵ�����ڣ��򲻽��д���ֱ�ӷ���
        return;
    }

    //��ʼ�Ƴ�����ҵ
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

	if(now == NULL){ //δ�ҵ�ǰ̨��ҵ�������fgPid���ǰ̨��ҵ
	    now = addJob(fgPid);
	}
    
        //�޸�ǰ̨��ҵ��״̬����Ӧ�������ʽ������ӡ��ʾ��Ϣ
        strcpy(now->state, STOPPED); 
        now->cmd[strlen(now->cmd)] = '&';
        now->cmd[strlen(now->cmd) + 1] = '\0';
        printf("\n[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);
        //����SIGSTOP�źŸ�����ǰ̨�����Ĺ���������ֹͣ
        kill(fgPid, SIGSTOP);
        fgPid = 0;
}

/*��ϼ�ctrl+c*/
void ctrl_C(){
    if (fgPid == 0) return;
    else kill(fgPid, SIGKILL);
}

/*fg����*/
void fg_exec(int pid){    
    Job *now = NULL; 
    int i;
    

    //����pid������ҵ
    now = head;
    while(now != NULL && now->pid != pid){
        now = now->next;
    }
    
    if(now == NULL){ //δ�ҵ���ҵ
        printf("pidΪ%d ����ҵ�����ڣ�\n", pid);
        return;
    }

    //��¼ǰ̨��ҵ��pid���޸Ķ�Ӧ��ҵ״̬
    fgPid = now->pid;
    strcpy(now->state, RUNNING);

    i = strlen(now->cmd) - 1;
    while(i >= 0 && now->cmd[i] != '&'){
        i--;
    }
    now->cmd[i] = '\0';

    kill(now->pid, SIGCONT); //�������ҵ����SIGCONT�źţ�ʹ������
    sleep(1);
    waitpid(fgPid, NULL, WUNTRACED); //�����̵ȴ�ǰ̨���̵�����
}

/*bg����*/
void bg_exec(int pid){
    Job *now = NULL;

    //����pid������ҵ
    now = head;
    while(now != NULL && now->pid != pid){
        now = now->next;
    }
    if(now == NULL){ //δ�ҵ���ҵ
        printf("pidΪ%d ����ҵ�����ڣ�\n", pid);
        return;
    }
    
    strcpy(now->state, RUNNING); //�޸Ķ�����ҵ��״̬
    printf("[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);
    kill(now->pid, SIGCONT); //�������ҵ����SIGCONT�źţ�ʹ������
}

/*******************************************************
                    ������ʷ��¼
********************************************************/
void addHistory(char *cmd){
    if(history.end == -1){ //��һ��ʹ��history����
        history.end = 0;
        strcpy(history.cmds[history.end], cmd);
        return;
    }
    
    history.end = (history.end + 1)%HISTORY_LEN; //endǰ��һλ
    strcpy(history.cmds[history.end], cmd); //���������endָ���������
    
    if(history.end == history.start){ //end��startָ��ͬһλ��
        history.start = (history.start + 1)%HISTORY_LEN; //startǰ��һλ
    }
}

/*******************************************************
                     ��ʼ������
********************************************************/
/*ͨ��·���ļ���ȡ����·��*/
void getEnvPath(int len, char *buf){
    int i, j, last = 0, pathIndex = 0, temp;
    char path[40];
    
    for(i = 0, j = 0; i < len; i++){
        if(buf[i] == ':'){ //����ð��(:)�ָ��Ĳ���·���ֱ����õ�envPath[]��
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

/*��ʼ������*/
void init(){
    int fd, len, pid;
    char c, buf[80];

    //�򿪲���·���ļ�atea-sh.conf
    if((fd = open("atea-sh.conf", O_RDONLY, 660)) == -1){
        perror("init environment failed\n");
        exit(1);
    }
    
    //��ʼ��history����
    history.end = -1;
    history.start = 0;
    
    len = 0;
    //��·���ļ��������ζ��뵽buf[]��
    while(read(fd, &c, 1) != 0){ 
        buf[len++] = c;
    }
    close(fd);
    buf[len] = '\0';

    //������·������envPath[]
    getEnvPath(len, buf); 
    
    //ע���ź�
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
                      ����ִ��
********************************************************/
/*�ض���*/
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
    if(cmd->pflag == 0){ //���ùܵ�
        if(cmd->input != NULL && (i = open(cmd->input, O_RDONLY, S_IRUSR|S_IWUSR)) == -1){
            printf("���ܴ��ļ� %s��\n", cmd->input);
            return;
        }
        if(cmd->output != NULL && (o = open(cmd->output, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1){
            printf("���ܴ��ļ� %s��\n", cmd->output);
            return ;
        }
        if (pid == 0){
            //close(fildes[0]);
            //close(fildes[1]);
            while (read(fildes[1], buf, PIPEBUFSIZE) > 0);
            pPipe(cmd, i, o);
            ioAlloc(i, o);
        }
    } else if (cmd->pflag == 1){ //ֻ�ùܵ���         
        if(cmd->output != NULL && (o = open(cmd->output, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1){
            printf("���ܴ��ļ� %s��\n", cmd->output);
            return ;
        }
        i = fildes[0];
        if (pid == 0){
            //close(fildes[1]);
            pPipe(cmd, i, o);
            ioAlloc(i, o);
        }
    } else if (cmd->pflag == 2){ //ֻ�ùܵ�д
        if(cmd->input != NULL && (i = open(cmd->input, O_RDONLY, S_IRUSR|S_IWUSR)) == -1){
            printf("���ܴ��ļ� %s��\n", cmd->input);
            return;
        }
        o = fildes[1];
        if (pid == 0){
            //close(fildes[0]);
            while (read(fildes[1], buf, PIPEBUFSIZE) > 0);
            pPipe(cmd, i, o);
            ioAlloc(i, o);
        }
    } else { //�������ض���
        i = fildes[0];
        o = fildes[1];
        if (pid == 0){
            pPipe(cmd, i, o);
            ioAlloc(i, o);
        }        
    }
}


/*ִ���ⲿ����*/
void execOuterCmd(SimpleCmd *cmd){
    pid_t pid;
    int i;
    if(exists(cmd->args[0])){ //�������
        if((pid = fork()) < 0){
            perror("fork failed");
            return;
        }

        ioDirec(cmd, pid);
        if(pid == 0){ //�ӽ���
            if (cmd->pflag == 0 || cmd->pflag == 1){
                setpgid(0, 0);
            } else {
                setpgid(0, chainGid);
            }
            kill(getppid(), SIGUSR1);
            if(cmd->isBack){ //���Ǻ�̨��������ȴ�������������ҵ
                while(blocked); //�ȴ������̽���ҵ��ӵ�������
                blocked = 1;
                printf("[%d]\t%s\t\t%s\n", getpid(), RUNNING, inputBuff);
                kill(getppid(), SIGUSR1); //��Ϣ�����
            }
            justArgs(cmd->args[0]);
            if(execv(cmdBuff, cmd->args) < 0){ //ִ������
                printf("execv failed!\n");
                return;
            }
        } else { //������
            while (blocked);
            blocked = 1;
            if (cmd->pflag == 0 || cmd->pflag == 1) chainGid = getpgid(pid);
            if (cmd ->isBack){ //��̨����
                fgPid = 0; //pid��0��Ϊ��һ������׼��
                addJob(pid); //�����µ���ҵ
                kill(pid, SIGUSR1); //֪ͨ�ӽ�����ҵ�Ѽ���
                while (blocked); //�ȴ��ӽ������
                blocked = 1;
            } else { //�Ǻ�̨����
                fgPid = pid;
                tcsetpgrp(0, chainGid);
                waitpid(pid, NULL, WUNTRACED);
                tcsetpgrp(0, getpgrp());
            }
        }
    } else { //�������
        printf("�Ҳ�������%s\n", inputBuff);
    }
}

/*ִ������*/

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
        printf("��δִ���κ�����\n");
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
        printf("�����κ���ҵ\n");
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
            printf("cd; %s ������ļ������ļ�������\n", temp);
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
        printf("fg; �������Ϸ�����ȷ��ʽΪ��fg %<int>\n");
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
        printf("bg; �������Ϸ�����ȷ��ʽΪ��bg %<int>\n");
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
    if(strcmp(cmd->args[0], "exit") == 0) { //exit����
        bi_exit(cmd);
    } else if (strcmp(cmd->args[0], "history") == 0) {//history����
        bi_history(cmd);
    } else if (strcmp(cmd->args[0], "jobs") == 0) { //jobs����
        bi_jobs(cmd);
    } else if (strcmp(cmd->args[0], "cd") == 0) { //cd����
        bi_cd(cmd);
    } else if (strcmp(cmd->args[0], "fg") == 0) { //fg����
        bi_fg(cmd);
    } else if (strcmp(cmd->args[0], "bg") == 0) { //bg����
        bi_bg(cmd);
    } else if (strcmp(cmd->args[0], "echo") == 0){ //echo����
        bi_echo(cmd);
    } else if (strcmp(cmd->args[0], "exec") == 0){
        bi_exec(cmd);
    } else { //�ⲿ����
        execOuterCmd(cmd);
    }
}

/******************************************************
                     ����ִ�нӿ�
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

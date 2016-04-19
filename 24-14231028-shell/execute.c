#include <string.h>
#include <stdio.h>
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
int goon = 0, ingnore = 0;       //��������signal�ź���
char *envPath[10], cmdBuff[40];  //�ⲿ����Ĵ��·������ȡ�ⲿ����Ļ���ռ�
History history;                 //��ʷ����
Job *head = NULL;                //��ҵͷָ��
pid_t fgPid;                     //��ǰǰ̨��ҵ�Ľ��̺�

/*******************************************************
                  �����Լ���������
********************************************************/
/*�ж������Ƿ����*/
int exists(char *cmdFile){
    //printf("1\n");
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
    //printf("2\n");
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

/*���������ⲿ����ĸ�ʽ*/
void justArgs(char *str){
    //printf("3\n");
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

/*����goon*/
void setGoon(){
   //printf("4\n");
    goon = 1;
}

/*�ͷŻ��������ռ�*/
void release(){
//printf("5");
    int i;
    for(i = 0; strlen(envPath[i]) > 0; i++){
        free(envPath[i]);
    }
}

/*******************************************************
                  �ź��Լ�jobs���
********************************************************/
/*����µ���ҵ*/
Job* addJob(pid_t pid,int isBack){
//printf("6\n");
    Job *now = NULL, *last = NULL, *job = (Job*)malloc(sizeof(Job));
    
    //��ʼ���µ�job
    job->pid = pid;
    job->isBack=isBack;
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
//printf("7\n");
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
    
    if(now == NULL){ //��ҵ�����ڣ��򲻽��д���ֱ�ӷ���
        return;
    }
    if(now->isBack){
          while(
              waitpid(now->pid, NULL, WUNTRACED)==-1
          );
    }
	//��ʼ�Ƴ�����ҵ
    if(now == head){
        head = now->next;
    }else{
        last->next = now->next;
    }
    
    free(now);
}
/*��ϼ�����ctrl+c*/
void ctrl_C(){
//printf("8\n");
    Job *now = NULL;
    
    if(fgPid == 0){ //ǰ̨û����ҵ��ֱ�ӷ���
        return;
    }
    
    //SIGINT�źŲ�����ctrl+c
    ingnore = 2;
    
	now = head;
	while(now != NULL && now->pid != fgPid)
		now = now->next;
    
    if(now == NULL){ //δ�ҵ�ǰ̨��ҵ�������fgPid���ǰ̨��ҵ
        now = addJob(fgPid,0);
    }
    
	//�޸�ǰ̨��ҵ��״̬����Ӧ�������ʽ������ӡ��ʾ��Ϣ
    strcpy(now->state, KILL); 
    now->cmd[strlen(now->cmd)] = '&';
    now->cmd[strlen(now->cmd) + 1] = '\0';
    printf("[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);
    
	//����SIGSTOP�źŸ�����ǰ̨�����Ĺ�����ɱ������
    kill(fgPid, SIGINT);
    fgPid = 0;
}
/*��ϼ�����ctrl+z*/
void ctrl_Z(){
//printf("9\n");
    Job *now = NULL;
    
    if(fgPid == 0){ //ǰ̨û����ҵ��ֱ�ӷ���
        return;
    }
    
    //SIGCHLD�źŲ�����ctrl+z
    ingnore = 1;
    
	now = head;
	while(now != NULL && now->pid != fgPid)
		now = now->next;
    
    if(now == NULL){ //δ�ҵ�ǰ̨��ҵ�������fgPid���ǰ̨��ҵ
        now = addJob(fgPid,1);
    }
    
	//�޸�ǰ̨��ҵ��״̬����Ӧ�������ʽ������ӡ��ʾ��Ϣ
    strcpy(now->state, STOPPED); 
    now->cmd[strlen(now->cmd)] = '&';
    now->cmd[strlen(now->cmd) + 1] = '\0';
    printf("[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);
    
	//����SIGSTOP�źŸ�����ǰ̨�����Ĺ���������ֹͣ
    kill(fgPid, SIGSTOP);
    fgPid = 0;
}

/*fg����*/
void fg_exec(int pid){ 
//printf("10\n");   
    Job *now = NULL; 
	int i;
    //SIGCHLD�źŲ����Դ˺���
    ingnore = 1;
	//����pid������ҵ
    now = head;
	while(now != NULL && now->pid != pid)
		now = now->next;
    
    if(now == NULL){ //δ�ҵ���ҵ
        printf("pidΪ7%d ����ҵ�����ڣ�\n", pid);
        return;
    }

    //��¼ǰ̨��ҵ��pid���޸Ķ�Ӧ��ҵ״̬
    fgPid = now->pid;
    strcpy(now->state, RUNNING);
    now->isBack=0;
    signal(SIGTSTP, ctrl_Z); //����signal�źţ�Ϊ��һ�ΰ�����ϼ�Ctrl+Z��׼��
    signal(SIGINT, ctrl_C); //����signal�źţ�Ϊ��һ�ΰ�����ϼ�Ctrl+C��׼��
    i = strlen(now->cmd) - 1;
    while(i >= 0 && now->cmd[i] != '&')
		i--;
    now->cmd[i] = '\0';
    
    printf("%s\n", now->cmd);
    kill(now->pid, SIGCONT); //�������ҵ����SIGCONT�źţ�ʹ������
    sleep(1);
    waitpid(fgPid, NULL, 0); //�����̵ȴ�ǰ̨���̵�����
   // printf("haha");
}
/*bg����*/
void bg_exec(int pid){
//printf("11\n");
    Job *now = NULL;
    
    //SIGCHLD�źŲ����Դ˺���
    ingnore = 1;
    
	//����pid������ҵ
	now = head;
    while(now != NULL && now->pid != pid)
		now = now->next;
    if(now == NULL){ //δ�ҵ���ҵ
        printf("pidΪ7%d ����ҵ�����ڣ�\n", pid);
        return;
    }
    now->isBack = 1;
    strcpy(now->state, RUNNING); //�޸Ķ�����ҵ��״̬
    printf("[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);
    
    kill(now->pid, SIGCONT); //�������ҵ����SIGCONT�źţ�ʹ������
}
/*�ж��Ƿ�Ϊ�ⲿ�����ʵ��type*/
int type_exec(char *cmdFile){
        int i = 0;
    if(strcmp(cmdFile,"alias")==0 ||
        strcmp(cmdFile,"bg")==0 ||
        strcmp(cmdFile,"bind")==0 ||
        strcmp(cmdFile,"break")==0 ||
        strcmp(cmdFile,"builtin")==0 ||
        strcmp(cmdFile,"caller")==0 ||
        strcmp(cmdFile,"cd")==0 ||
        strcmp(cmdFile,"command")==0 ||
        strcmp(cmdFile,"compgen")==0 ||
        strcmp(cmdFile,"complete")==0 ||
        strcmp(cmdFile,"compopt")==0 ||
        strcmp(cmdFile,"continue")==0 ||
        strcmp(cmdFile,"declare")==0 ||
        strcmp(cmdFile,"dirs")==0 ||
        strcmp(cmdFile,"disown")==0 ||
        strcmp(cmdFile,"echo")==0 ||
        strcmp(cmdFile,"enable")==0 ||
        strcmp(cmdFile,"eval")==0 ||
        strcmp(cmdFile,"exec")==0 ||
        strcmp(cmdFile,"exit")==0 ||
        strcmp(cmdFile,"export")==0 ||
        strcmp(cmdFile,"false")==0 ||
        strcmp(cmdFile,"fc")==0 ||
        strcmp(cmdFile,"fg")==0 ||
        strcmp(cmdFile,"getopts")==0 ||
        strcmp(cmdFile,"hash")==0 ||
        strcmp(cmdFile,"help")==0 ||
        strcmp(cmdFile,"history")==0 ||
        strcmp(cmdFile,"jobs")==0 ||
        strcmp(cmdFile,"kill")==0 ||
        strcmp(cmdFile,"let")==0 ||
        strcmp(cmdFile,"local")==0 ||
        strcmp(cmdFile,"logout")==0 ||
        strcmp(cmdFile,"mapfile")==0 ||
        strcmp(cmdFile,"popd")==0 ||
        strcmp(cmdFile,"printf")==0 ||
        strcmp(cmdFile,"pushd")==0 ||
        strcmp(cmdFile,"pwd")==0 ||
        strcmp(cmdFile,"read")==0 ||
        strcmp(cmdFile,"readarray")==0 ||
        strcmp(cmdFile,"readonly")==0 ||
        strcmp(cmdFile,"return")==0 ||
        strcmp(cmdFile,"set")==0 ||
        strcmp(cmdFile,"shift")==0 ||
        strcmp(cmdFile,"shopt")==0 ||
        strcmp(cmdFile,"source")==0 ||
        strcmp(cmdFile,"suspend")==0 ||
        strcmp(cmdFile,"test")==0 ||
        strcmp(cmdFile,"times")==0 ||
        strcmp(cmdFile,"trap")==0 ||
        strcmp(cmdFile,"true")==0 ||
        strcmp(cmdFile,"type")==0 ||
        strcmp(cmdFile,"typeset")==0 ||
        strcmp(cmdFile,"ulimit")==0 ||
        strcmp(cmdFile,"umask")==0 ||
        strcmp(cmdFile,"unalias")==0 ||
        strcmp(cmdFile,"unset")==0 ||
        strcmp(cmdFile,"wait")==0){ //�����ڵ�ǰĿ¼
        strcpy(cmdBuff, cmdFile);
        return 1;
    }else{  //����ysh.conf�ļ���ָ����Ŀ¼��ȷ�������Ƿ����
        while(envPath[i] != NULL){ //����·�����ڳ�ʼ��ʱ������envPath[i]��
            strcpy(cmdBuff, envPath[i]);
            strcat(cmdBuff, cmdFile);
            if(access(cmdBuff, F_OK) == 0){ //�����ļ����ҵ�
                return 2;
            }
            
            i++;
        }
    }
    
    return 0; 
}

/*******************************************************
                    ������ʷ��¼
********************************************************/
void addHistory(char *cmd){
//printf("history\n");
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
//printf("12\n");
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
//printf("13\n");
    int fd, n, len;
    char c, buf[80];

	//�򿪲���·���ļ�ysh.conf
    if((fd = open("ysh.conf", O_RDONLY, 660)) == -1){
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
}

/*******************************************************
                      �������
********************************************************/
SimpleCmd* handleSimpleCmdStr(int begin, int end){
//printf("14\n");
    int i, j, k,l=1;
    int fileFinished; //��¼�����Ƿ�������
    char c, buff[10][40], inputFile[30], outputFile[30],pipebuff[30], *temp = NULL;
    SimpleCmd *cmd = (SimpleCmd*)malloc(sizeof(SimpleCmd));
    
	//Ĭ��Ϊ�Ǻ�̨�����������ض���Ϊnull
    cmd->isBack = 0;
    cmd->input = cmd->output =cmd->pipes= NULL;
    
    //��ʼ����Ӧ����
    for(i = begin; i<10; i++){
        buff[i][0] = '\0';
    }
    inputFile[0] = '\0';
    outputFile[0] = '\0';
    pipebuff[0]='\0';
    
    i = begin;
	//�����ո��������Ϣ
    while(i < end && (inputBuff[i] == ' ' || inputBuff[i] == '\t')){
        i++;
    }
    
    k = 0;
    j = 0;
    fileFinished = 0;
    temp = buff[k]; //����ͨ��tempָ����ƶ�ʵ�ֶ�buff[i]��˳�θ�ֵ����
    while(i < end){
		/*���������ַ��Ĳ�ͬ������в�ͬ�Ĵ���*/
        switch(inputBuff[i]){ 
            /*case '|': //�ܵ�
                printf("�ܵ�����");
                if(j != 0){
                    temp[j] = '\0';
                    j = 0;
                    if(!fileFinished){
                        k++;
                        temp = buff[k];
                    }
                }
                fileFinished = 1;
                i++;*/
            case ' ':
            case '\t': //�������������Ľ�����־
                temp[j] = '\0';
                j = 0;
                if(!fileFinished){
                    k++;
                    temp = buff[k];
                }
                break;

            case '<': //�����ض����־
                if(j != 0){
		    //���ж�Ϊ��ֹ����ֱ�Ӱ���<���ŵ����ж�Ϊͬһ�����������ls<sth
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
                
            case '>': //����ض����־
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
                
            case '&': //��̨���б�־
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
	    case '|':
                l=0;
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
            default: //Ĭ������뵽tempָ���Ŀռ�
                temp[j++] = inputBuff[i++];
                continue;
	}
        
		//�����ո��������Ϣ
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
    
	//����Ϊ�������������������ֵ
    cmd->args = (char**)malloc(sizeof(char*) * (k + 1));
    cmd->args[k] = NULL;
    for(i = 0; i<k; i++){
        j = strlen(buff[i]);
        cmd->args[i] = (char*)malloc(sizeof(char) * (j + 1));   
        strcpy(cmd->args[i], buff[i]);
    }
    
	//����������ض����ļ�����Ϊ����������ض��������ֵ
    if(strlen(inputFile) != 0){
        j = strlen(inputFile);
        cmd->input = (char*)malloc(sizeof(char) * (j + 1));
        strcpy(cmd->input, inputFile);
    }
    if(l== 0){ //����йܵ�����Ϊ����Ĺܵ�������ֵ
	cmd->pipes = (char**)malloc(sizeof(char*)*2);
	cmd->pipes[1] = NULL;
	for(i = 0; i<k; i++){
        j = strlen(buff[i]);
        cmd->pipes[i] = (char*)malloc(sizeof(char) * (j + 1));   
        strcpy(cmd->pipes[i], buff[i]);
    }
    }
    //���������ض����ļ�����Ϊ���������ض��������ֵ
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
                      ����ִ��
********************************************************/
/*ִ���ⲿ����*/
void execOuterCmd(SimpleCmd *cmd){
//printf("15\n");
    pid_t pid;
    int pipeIn, pipeOut;
    
    if(exists(cmd->args[0])){ //�������
	if(cmd->pipes ==NULL){
       	if((pid = fork()) < 0){
            perror("fork failed");
            return;
        }
        //printf("haha:%d\n",pid);
        if(pid == 0){ //�ӽ���
            //printf("Child process begin\n");
            setpgrp();
            if(cmd->input != NULL){ //���������ض���
                if((pipeIn = open(cmd->input, O_RDONLY, S_IRUSR|S_IWUSR)) == -1){
                    printf("���ܴ��ļ� %s��\n", cmd->input);
                    return;
                }
                if(dup2(pipeIn, 0) == -1){
                    printf("�ض����׼�������\n");
                    return;
                }
            }
            
            if(cmd->output != NULL){ //��������ض���
                if((pipeOut = open(cmd->output, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1){
                    printf("���ܴ��ļ� %s��\n", cmd->output);
                    return ;
                }
                if(dup2(pipeOut, 1) == -1){
                    printf("�ض����׼�������\n");
                    return;
                }
            }
	    
            
            if(cmd->isBack){ //���Ǻ�̨��������ȴ�������������ҵ
                signal(SIGUSR1, setGoon); //�յ��źţ�setGoon������goon��1�������������ѭ��
                while(goon == 0) {
                    sleep(1);
                    printf("���ܴ��ļ�%s\n",cmd);
                } //�ȴ�������SIGUSR1�źţ���ʾ��ҵ�Ѽӵ�������
                goon = 0; //��0��Ϊ��һ������׼��
                
                printf("[%d]\t%s\t\t%s\n", getpid(), RUNNING, inputBuff);
                kill(getppid(), SIGUSR1);
                //printf("Send singal to father process\n");
            }
            
            justArgs(cmd->args[0]);
            if(execv(cmdBuff, cmd->args) < 0){ //ִ������
                printf("execv failed!\n");
                return;
            }
        }
	else{ //������
            //printf("father begin\n");
            if(cmd ->isBack){ //��̨����             
                fgPid = 0; //pid��0��Ϊ��һ������׼��
                addJob(pid,1); //�����µ���ҵ
                sleep(1);//��ֹkill����ɱ���ӽ���
                kill(pid, SIGUSR1); //�ӽ��̷��źţ���ʾ��ҵ�Ѽ���
                //printf("father 1\n");
                //�ȴ��ӽ������
                signal(SIGUSR1, setGoon);
                //printf("father 2\n");
                while(goon == 0) {
                    sleep(1);
                    //printf("Wait child process\n");
                };
                goon = 0;
            }else{ //�Ǻ�̨����
                fgPid = pid;
                waitpid(pid, NULL, 0);
                //printf("haha");
            }
		}
	}
	else{
	    int pid[2];
	    int pipe_fd[2];
	    int l,i,j;
	    char *prog2_argv[3];
            char *prog1_argv[3];
            prog1_argv[0]=cmd->args[0];
            prog1_argv[1]=cmd->args[1];
            prog1_argv[2]=NULL;
	    prog2_argv[0] = cmd->args[3];
	    prog2_argv[1] = cmd->args[4];
            prog2_argv[2]=NULL;
	    //�����ܵ�
	    if(pipe(pipe_fd) < 0){
		perror("pipe failed");
		exit(errno);
	    }
	    //Ϊ��һ�����������
	    if((pid[0] = fork()) < 0){
		perror("Fork failed");
           	exit(errno);
	    }
	    if(!pid[0]){
                /*���ܵ���д���������Ƹ���׼����*/
		close(pipe_fd[0]);
		dup2(pipe_fd[1],1);
		close(pipe_fd[1]);
		execvp(prog1_argv[0],prog1_argv);
	    }
	    if(pid[0]){
		if((pid[1]=fork()) < 0){
		    perror("Fork failed");
		    exit(errno);
		}
		if(!pid[1]){
//printf("ndflasfad");
		    close(pipe_fd[1]);
		    dup2(pipe_fd[0],0);
		    close(pipe_fd[0]);
		    execvp(prog2_argv[0],prog2_argv);
		}
	    close(pipe_fd[0]);
	    close(pipe_fd[1]);
	}
	}
	
    }else{ //�������
        printf("�Ҳ������� 15%s\n", inputBuff);
    }
}

/*ִ������*/
void execSimpleCmd(SimpleCmd *cmd){
//printf("16\n");
    int i, pid;
    char *temp;
    Job *now = NULL;
    
    if(strcmp(cmd->args[0], "exit") == 0) { //exit����
        exit(0);
    } else if (strcmp(cmd->args[0], "history") == 0) { //history����
        if(history.end == -1){
            printf("��δִ���κ�����\n");
            return;
        }
        i = history.start;
        do {
            printf("%s\n", history.cmds[i]);
            i = (i + 1)%HISTORY_LEN;
        } while(i != (history.end + 1)%HISTORY_LEN);
    } else if (strcmp(cmd->args[0], "jobs") == 0) { //jobs����
        if(head == NULL){
            printf("�����κ���ҵ\n");
        } else {
            printf("index\tpid\tstate\t\tcommand\n");
            for(i = 1, now = head; now != NULL; now = now->next, i++){
                printf("%d\t%d\t%s\t\t%s\n", i, now->pid, now->state, now->cmd);
            }
        }
    } else if (strcmp(cmd->args[0], "cd") == 0) { //cd����
        temp = cmd->args[1];
        if(temp != NULL){
            if(chdir(temp) < 0){
                printf("cd; %s ������ļ������ļ�������\n", temp);
            }
        }
    } else if (strcmp(cmd->args[0], "fg") == 0) { //fg����
        temp = cmd->args[1];
        if(temp != NULL && temp[0] == '%'){
            pid = str2Pid(temp, 1, strlen(temp));
            if(pid != -1){
                fg_exec(pid);
            }
        }else{
            printf("fg; �������Ϸ�����ȷ��ʽΪ��fg %<int>\n");
        }
    } else if (strcmp(cmd->args[0], "bg") == 0) { //bg����
        temp = cmd->args[1];
        if(temp != NULL && temp[0] == '%'){
            pid = str2Pid(temp, 1, strlen(temp));
            
            if(pid != -1){
                bg_exec(pid);
            }
        }
	else{
            printf("bg; �������Ϸ�����ȷ��ʽΪ��bg %<int>\n");
        }
    } else if(strcmp(cmd->args[0], "pwd") == 0){//pwd����
         
            printf("%s\n",getcwd(NULL,0));
         

      }else if(strcmp(cmd->args[0], "echo") == 0){//echo����
            temp  = cmd->args[1];
            if(temp==NULL){
                 printf("û����������ݣ���\n");
            }
            else{
                 if(strcmp(cmd->args[1],"-n")==0){
                     for(i = 2; cmd->args[i] != NULL; i++){
    		         printf("%s ",cmd->args[i]);
	             }
                 }
                 else{
                     for(i = 1; cmd->args[i] != NULL; i++){
    		         printf("%s ",cmd->args[i]);
	             }
                     printf("\n");
                 }
             }
        }else if(strcmp(cmd->args[0], "type") == 0){//type����
             if(type_exec(cmd->args[1])==2){
                 printf("%s out\n",cmd->args[1]);  
             }
             else if(type_exec(cmd->args[1])==1){
                 printf("%s in\n",cmd->args[1]);
             }
             else{
                 printf("NO!!\n");
             }
        }else{ //�ⲿ����
        execOuterCmd(cmd);
    }
    
    //�ͷŽṹ��ռ�
    for(i = 0; cmd->args[i] != NULL; i++){
        free(cmd->args[i]);
        free(cmd->input);
        free(cmd->output);
    }
}

/*******************************************************
                     ����ִ�нӿ�
********************************************************/
void execute(){
//printf("19\n");

    SimpleCmd *cmd = handleSimpleCmdStr(0, strlen(inputBuff));
//printf("aa a a aa aa aa%s\n",inputBuff);
    execSimpleCmd(cmd);
}

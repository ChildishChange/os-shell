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
int goon = 0, ingnore = 0, status, split = 0;       //��������signal�ź���
char *envPath[10], cmdBuff[40];  //�ⲿ����Ĵ��·������ȡ�ⲿ����Ļ���ռ�
History history;                 //��ʷ����
Job *head = NULL;                //��ҵͷָ��
pid_t fgPid;                     //��ǰǰ̨��ҵ�Ľ��̺�

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
                return i + 2;
            }
            
            i++;
        }
    }
    
    return 0; 
}

/*���ַ���ת��Ϊ���͵�Pid*/
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

/*����goon*/
void setGoon(){
    goon = 1;
}

/*�ͷŻ��������ռ�*/
void release(){
    int i;
    for(i = 0; strlen(envPath[i]) > 0; i++){
        free(envPath[i]);
    }
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
 
    pid = sip->si_pid;
    now = head;
    while(now != NULL && now->pid < pid){
	last = now;
	now = now->next;
    }
    if(now == NULL){ //��ҵ�����ڣ��򲻽��д���ֱ�ӷ���
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
	//��ʼ�Ƴ�����ҵ
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

/*��ϼ�����ctrl+z*/
void ctrl_Z(){

    Job *now = NULL;
    
    if(fgPid == 0){ //ǰ̨û����ҵ��ֱ�ӷ���
        return;
    }
    
    //SIGCHLD�źŲ�����ctrl+z
    
	now = head;
	while(now != NULL && now->pid != fgPid)
		now = now->next;
    if(now == NULL){ //δ�ҵ�ǰ̨��ҵ�������fgPid���ǰ̨��ҵ
        now = addJob(fgPid);
    }
	//�޸�ǰ̨��ҵ��״̬����Ӧ�������ʽ������ӡ��ʾ��Ϣ
    strcpy(now->state, STOPPED); 
    int l = strlen(now->cmd);
    now->cmd[l] = '&';
    now->cmd[l + 1] = '\0';
    printf("[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);

	//����SIGSTOP�źŸ�����ǰ̨�����Ĺ���������ֹͣ
    
    kill(-fgPid, SIGSTOP);
    tcsetpgrp(0, getpid());
    fgPid = 0;
}

/*��ϼ�����ctrl+c*/
void ctrl_C(){
	
    Job *now = NULL;
    
    if(fgPid == 0){ //ǰ̨û����ҵ��ֱ�ӷ���
        return;
    }
    
    //SIGCHLD�źŲ�����ctrl+c
    
	now = head;
	while(now != NULL && now->pid != fgPid)
		now = now->next;
    if(now == NULL){ //δ�ҵ�ǰ̨��ҵ�������fgPid���ǰ̨��ҵ
        now = addJob(fgPid);
    }
    //����SIGKILL�źŸ�����ǰ̨�����Ĺ���
    kill(-fgPid, SIGKILL);
    tcsetpgrp(0, getpid());
    fgPid = 0;
}

/*fg����*/
void fg_exec(int pid){    
    Job *now = NULL; 
	int i;
    
    //SIGCHLD�źŲ����Դ˺���
    
	//����pid������ҵ
    now = head;
	while(now != NULL && now->pid != pid)
		now = now->next;
    
    if(now == NULL){ //δ�ҵ���ҵ
        printf("pid: %-7d is not exist!\n", pid);
        return;
    }

    if(strcmp(now->state, RUNNING)){
	ingnore = 1;
    }

    //��¼ǰ̨��ҵ��pid���޸Ķ�Ӧ��ҵ״̬
    fgPid = now->pid;
    strcpy(now->state, RUNNING);
    
    signal(SIGTSTP, ctrl_Z); //����signal�źţ�Ϊ��һ�ΰ�����ϼ�Ctrl+Z��׼��
    i = strlen(now->cmd) - 1;
    while(i >= 0 && now->cmd[i] != '&')
		i--;
    now->cmd[i] = '\0';
    
    printf("%s\n", now->cmd);

    tcsetpgrp(0, fgPid);
    kill(-now->pid, SIGCONT); //�������ҵ����SIGCONT�źţ�ʹ������
    while(waitpid(fgPid, &status, WUNTRACED) == -1);
    tcsetpgrp(0, getpid());
}

/*bg����*/
void bg_exec(int pid){
    Job *now = NULL;
    
    //SIGCHLD�źŲ����Դ˺���
    ingnore = 1;
    
	//����pid������ҵ
	now = head;
    while(now != NULL && now->pid != pid)
		now = now->next;
    
    if(now == NULL){ //δ�ҵ���ҵ
        printf("pid: %-7d is not exist!\n", pid);
        return;
    }
    
    strcpy(now->state, RUNNING); //�޸Ķ�����ҵ��״̬
    printf("[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);
    
    kill(-now->pid, SIGCONT); //�������ҵ����SIGCONT�źţ�ʹ������
	
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

    struct sigaction action2;
    action2.sa_handler = SIG_IGN;
    sigaction(SIGTTOU, &action2, NULL);

    signal(SIGTSTP, ctrl_Z);
    signal(SIGINT, ctrl_C);
}

/*******************************************************
                      �������
********************************************************/
SimpleCmd* handleSimpleCmdStr(int begin, int end){
    int i, j, k;
    int fileFinished; //��¼�����Ƿ�������
    char c, buff[10][40], inputFile[30], outputFile[30], *temp = NULL;
    SimpleCmd *cmd = (SimpleCmd*)malloc(sizeof(SimpleCmd));
    
	//Ĭ��Ϊ�Ǻ�̨�����������ض���Ϊnull
    cmd->isBack = 0;
    cmd->input = cmd->output = NULL;
    //��ʼ����Ӧ����
    for(i = begin; i<10; i++){
        buff[i][0] = '\0';
    }
    inputFile[0] = '\0';
    outputFile[0] = '\0';
    
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

SimpleCmd* handleSimpleCmdStr0(int begin, int end){
    int i, j, k;
    int fileFinished; //��¼�����Ƿ�������
    char c, buff[10][40], inputFile[30], outputFile[30], *temp = NULL;
    SimpleCmd *cmd = (SimpleCmd*)malloc(sizeof(SimpleCmd));

	//Ĭ��Ϊ�Ǻ�̨�����������ض���Ϊnull
    cmd->isBack = 0;
    cmd->input = cmd->output = NULL;
    
    //��ʼ����Ӧ����
    for(i = begin; i<10; i++){
        buff[i][0] = '\0';
    }
    inputFile[0] = '\0';
    outputFile[0] = '\0';
    
    i = begin;
	//�����ո��������Ϣ
    while(i < end && (inputbuff[i] == ' ' || inputbuff[i] == '\t')){
        i++;
    }
    
    k = 0;
    j = 0;
    fileFinished = 0;
    temp = buff[k]; //����ͨ��tempָ����ƶ�ʵ�ֶ�buff[i]��˳�θ�ֵ����
    while(i < end){
		
        switch(inputbuff[i]){ 
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
                
            default: //Ĭ������뵽tempָ���Ŀռ�
                temp[j++] = inputbuff[i++];
                continue;
		}
        
		//�����ո��������Ϣ
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

    //���������ض����ļ�����Ϊ���������ض��������ֵ
    if(strlen(outputFile) != 0){
        j = strlen(outputFile);
        cmd->output = (char*)malloc(sizeof(char) * (j + 1));   
        strcpy(cmd->output, outputFile);
    }

    return cmd;
	
}
/*******************************************************
                      ����ִ��
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
       //�ѵ�ǰĿ¼.����һ��Ŀ¼..�������ļ���ȥ����������ѭ������Ŀ¼
        if(strncmp(file->d_name, ".", 1) == 0)
            continue;
        strcpy(filename[len++], file->d_name);//������������ļ���
       
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
        
    if(pid == 0) { //�ӽ���

	if(cmd->isBack){ //���Ǻ�̨��������ȴ�������������ҵ
		kill(getppid(), SIGUSR1);

	    	signal(SIGUSR1, setGoon); 	
                while(goon == 0) ; //�ȴ�������SIGUSR1�źţ���ʾ��ҵ�Ѽӵ�������
                goon = 0; //��0��Ϊ��һ������׼��
                
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
    else { //������

      	if(cmd ->isBack){ //��̨���� 
		signal(SIGUSR1, setGoon);  
		while(goon == 0) ;
                goon = 0;
          
                fgPid = 0; //pid��0��Ϊ��һ������׼��
                addJob(pid); //�����µ���ҵ
                kill(pid, SIGUSR1); //�ӽ��̷��źţ���ʾ��ҵ�Ѽ���
                
                //�ȴ��ӽ������
                signal(SIGUSR1, setGoon);
                while(goon == 0) ;
                goon = 0;
            }else{ //�Ǻ�̨����
		signal(SIGUSR1, setGoon);  
		while(goon == 0) ;
                    goon = 0;

		tcsetpgrp(0, pid);
		fgPid = pid;
		addJob(pid); //�����µ���ҵ

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

/*ִ���ⲿ����*/
void execOuterCmd(SimpleCmd *cmd){
    pid_t pid;
    int pipeIn, pipeOut;

    if(exists(cmd->args[0])){ //�������

        if((pid = fork()) < 0){
            perror("fork failed");
            return;
        }
        
        if(pid == 0){ //�ӽ���
            if(cmd->input != NULL){ //���������ض���
                if((pipeIn = open(cmd->input, O_RDONLY, S_IRUSR|S_IWUSR)) == -1){
                    printf("Can't open file %s!\n", cmd->input);
                    return;
                }
                if(dup2(pipeIn, 0) == -1){
                    printf("Redirect standard input error!\n");
                    return;
                }
            }
            
            if(cmd->output != NULL){ //��������ض���
                if((pipeOut = open(cmd->output, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1){
                    printf("Can't open file %s!\n", cmd->output);
                    return ;
                }
                if(dup2(pipeOut, 1) == -1){
                    printf("Redirect standard output error!\n");
                    return;
                }
            }
            
            if(cmd->isBack){ //���Ǻ�̨��������ȴ�������������ҵ
		kill(getppid(), SIGUSR1);

                signal(SIGUSR1, setGoon);	
                while(goon == 0) ; //�ȴ�������SIGUSR1�źţ���ʾ��ҵ�Ѽӵ�������
                goon = 0; //��0��Ϊ��һ������׼��
                
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
            if(execv(cmdBuff, cmd->args) < 0){ //ִ������
                printf("execv failed!\n");
                _exit(0);
            }
        }
	else{ //������
            if(cmd ->isBack){ //��̨���� 
		signal(SIGUSR1, setGoon);
		while(goon == 0) ;
                goon = 0;
          
                fgPid = 0; //pid��0��Ϊ��һ������׼��
                addJob(pid); //�����µ���ҵ
                kill(pid, SIGUSR1); //�ӽ��̷��źţ���ʾ��ҵ�Ѽ���
                
                //�ȴ��ӽ������
                signal(SIGUSR1, setGoon);
                while(goon == 0) ;
                goon = 0;
            }
	    else{ //�Ǻ�̨����
		signal(SIGUSR1, setGoon);  
		while(goon == 0) ;
                    goon = 0;

		tcsetpgrp(0, pid);
	        fgPid = pid;
		addJob(pid); //�����µ���ҵ

		kill(pid, SIGUSR1);
		waitpid(pid, &status, WUNTRACED);
		tcsetpgrp(0, getpid());
            }
	}
    } else{ //�������
        printf("Command is not exist %-15s\n", inputBuff);
    }
}

/*ִ������*/
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
    } else if(strcmp(cmd->args[0], "exit") == 0){ //exit����
        exit(0);
    } else if (strcmp(cmd->args[0], "history") == 0){ //history����
        if(history.end == -1){
            printf("There is no command\n");
            return;
        }
        i = history.start;
        do {
            printf("%s\n", history.cmds[i]);
            i = (i + 1)%HISTORY_LEN;
        } while(i != (history.end + 1)%HISTORY_LEN);
    } else if (strcmp(cmd->args[0], "jobs") == 0){ //jobs����
        if(head == NULL){
            printf("There is no work\n");
        } else {
            printf("index\tpid\tstate\t\tcommand\n");
            for(i = 1, now = head; now != NULL; now = now->next, i++){
                printf("%d\t%d\t%s\t\t%s\n", i, now->pid, now->state, now->cmd);
            }
        }
    } else if (strcmp(cmd->args[0], "cd") == 0){ //cd����
        temp = cmd->args[1];
        if(temp != NULL){
            if(chdir(temp) < 0){
                printf("cd; %s Wrong file or folder!\n", temp);
            }
        }
    } else if (strcmp(cmd->args[0], "fg") == 0){ //fg����
        temp = cmd->args[1];
        if(temp != NULL && temp[0] == '%'){
            pid = str2Pid(temp, 1, strlen(temp));
            if(pid != -1){
                fg_exec(pid);
            }
        }else{
            printf("fg; Parameter is not valid, the correct format is: fg %<int>\n");
        }
    } else if (strcmp(cmd->args[0], "bg") == 0){ //bg����
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
    } else if (strcmp(cmd->args[0], "echo") == 0){ //echo����
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
    } else if (strcmp(cmd->args[0], "kill") == 0){ //kill����
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
    } else if (strcmp(cmd->args[0], "type") == 0){ //type����
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
    } else{ //�ⲿ����
        execOuterCmd(cmd);
    }
    
    //�ͷŽṹ��ռ�
    for(i = 0; cmd->args[i] != NULL; i++){
        free(cmd->args[i]);
    }
    free(cmd->input);
    free(cmd->output);
 
}

/*******************************************************
                     ����ִ�нӿ�
********************************************************/
void execute(){
    SimpleCmd *cmd = handleSimpleCmdStr(0, strlen(inputBuff));
    execSimpleCmd(cmd);
}

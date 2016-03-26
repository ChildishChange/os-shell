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
#include <dirent.h>//jsz add
#include <sys/stat.h>//jsz add

#include "global.h"
//#define DEBUG
int goon = 0, ingnore = 0;       //用于设置signal信号量
char *envPath[10], cmdBuff[40];  //外部命令的存放路径及读取外部命令的缓冲空间
History history;                 //历史命令
Job *head = NULL;                //作业头指针
pid_t fgPid;                     //当前前台作业的进程号
int status;
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

/*设置子进程僵死处理函数*/
void sig_chld() {
    pid_t pid;
    int stat;
    pid = waitpid(-1,&stat,WNOHANG);
    return;
}

/*jsz add带通配符的字符串匹配函数*/
int match_imp(const char *d,int dcur,const char *s,int scur) {
    if(!d[dcur]) return (!s[scur])?1:0;
    if (d[dcur]=='?') return match_imp(d,dcur+1,s,scur+1);
    else if(d[dcur]=='*') {
        do {
            if (match_imp(d,dcur+1,s,scur)) return 1;
        } while (s[scur++]);//当s未结束时继续匹配
        return 0;
    }else return (tolower(d[dcur])==tolower(s[scur]) && match_imp(d,dcur+1,s,scur+1))?1:0;
}
int match(char* s1, char* s2) {//字符串匹配函数 s1带通配符，s2不带通配符
	int i,j;
	int len1,len2;
	int count1=0,count2=0;
	char news1[1000]={0};
	char news2[1000]={0};
	int newcount=0;
	int newcount2=0;
	int questioncount=0;
	len1=strlen(s1);
	len2=strlen(s2);
    if (match_imp(s1,0,s2,0)==1){//判断'/'数目是否相等
		for(i=0;i<len1;i++)
			if(s1[i]=='/')
				count1++;
		for(i=0;i<len2;i++)
			if(s2[i]=='/')
				count2++;
		//return count1==count2;
		if(count1==count2){//检查?
			if(strstr(s1,"?")==NULL)//不存在?
				return 1;
			if(strstr(s1,"/")!=NULL){//存在/
				while(s1[newcount]!=0&&s2[newcount]!=0){
					if(s1[newcount]!=s2[newcount]){
						news1[newcount2]=s1[newcount];
						news2[newcount2++]=s2[newcount];
					}
					newcount++;
				}
				if(s1[newcount]==0)
					for(j=newcount;j<len2;j++)
						news2[newcount2++]=s2[j];
				else
					for(j=newcount;j<len1;j++)
						news1[newcount2++]=s1[j];
			}
			else{
				strcpy(news1,s1);
				strcpy(news2,s2);
			}
			//开始检查?数量
			for(j=0;j<strlen(s1);j++)
				if(s1[j]=='?'||s1[j]!='*')
					questioncount++;
			if(questioncount>strlen(s2))
				return 0;
			else
				return 1;
				
		}
	}
	return 0;
}
/*jsz add字符串反转函数*/
void revstr(char* s1,char *s2){//字符串反转函数 s1反转后存入s2
	int i;
	int len=strlen(s1);
	for(i=0;i<len;i++)
		s2[len-1-i]=s1[i];
}
/*jsz add字符串排序函数*/
void strsort(char queue[][1000],int* qcount){
	int i,j;
	int len;
	char temp[1000]={0};
	len=(*qcount);
	for(i=0;i<len-1;i++)
		for(j=i+1;j<len;j++)
			if(strcmp(queue[i],queue[j])>0){
				strcpy(temp,queue[i]);
				strcpy(queue[i],queue[j]);
				strcpy(queue[j],temp);
			}
}
/*jsz add 通配符递归查找函数*/
void dirsearch(char* str,char* presentdir,char queue[][1000],int* qcount,char* relativepath){//递归查找函数
	//参数：str 带路径和通配符的字符串 presentdir 当前查找目录 queue匹配成功队列 qcount 队列元素数目 relativepath 相对路径
	DIR *dir;
	struct dirent *ptr;
	struct stat statbuf;
	char pathandname[1000];
	char newstr[1000];
	
	dir=opendir(presentdir);//打开目录
	while((ptr=readdir(dir))!=NULL){//ptr->d_name为每一个文件名
		//开始进行匹配
		snprintf(pathandname,1000,"%s/%s",presentdir,ptr->d_name);//为文件名带上目录信息
		if(match(str,pathandname)==1){//匹配成功			
			lstat(pathandname,&statbuf);//得到文件信息
			if(!S_ISDIR(statbuf.st_mode)){//判断是文件还是目录
				if(ptr->d_name[0]=='.')//文件名的第一个是.
					continue;
				snprintf(newstr,1000,"%s%s",relativepath,ptr->d_name);
				strcpy(queue[(*qcount)++],newstr);
			}
			else{
				if(strstr(pathandname,".")!=NULL)//目录中包含.
					continue;
				snprintf(newstr,1000,"%s%s",relativepath,ptr->d_name);
				strcpy(queue[(*qcount)++],newstr);
				/*暂时不需要递归查找
				char newrelativepath[1000];
				snprintf(newrelativepath,1000,"%s%s/",relativepath,ptr->d_name);
				dirsearch(str,pathandname,queue,qcount,newrelativepath);//进入子目录继续查找
				*/
			}
		}
		else{//进入目录进行递归查找
			lstat(pathandname,&statbuf);//得到文件信息
			if(S_ISDIR(statbuf.st_mode)&&strstr(pathandname,".")==NULL){//如果是目录且不包含.
				char newrelativepath[1000];
				snprintf(newrelativepath,1000,"%s%s/",relativepath,ptr->d_name);
				dirsearch(str,pathandname,queue,qcount,newrelativepath);//进入子目录继续查找
			}
		}
	}
	closedir(dir);//关闭目录
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
    //ingnore=1;
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



/*组合键命令ctrl+z*/
void ctrl_Z(){
    Job *now = NULL;
    if(MY_DEBUG) printf("in ctrl_z fgPid==%d\n",fgPid);
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
    signal(SIGINT,ctrl_Z);
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
	Job *now=NULL;
	
	if(fgPid==0)
		return;
	
	//SIGCHLD信号产生自ctrl+c,可以清除作业
    ingnore = 0;
    
	now=head;
	
	while(now!=NULL&&now->pid!=fgPid)
		now=now->next;
		
	if(now==NULL)
		now=addJob(fgPid);
		
    strcpy(now->state, KILLED);
    now->cmd[strlen(now->cmd)] = '\0';
    printf("[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);
    
	kill(fgPid,SIGKILL);
	fgPid=0;
}

/*fg命令*/
void fg_exec(int pid){    
    Job *now = NULL; 
	int i;
    //printf("hello\n");
    //SIGCHLD信号产生自此函数
    ingnore = 1;
    
	//根据pid查找作业
    now = head;
	while(now != NULL && now->pid != pid)
		now = now->next;
    
    if(now == NULL){ //未找到作业
        printf("pid为7%d 的作业不存在！\n", pid);
        return;
    }

    //记录前台作业的pid，修改对应作业状态
    fgPid = now->pid;
    strcpy(now->state, RUNNING);
    
    //signal(SIGTSTP, ctrl_Z); //设置signal信号，为下一次按下组合键Ctrl+Z做准备
    signal(SIGINT,ctrl_C);
    //printf("[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);
    i = strlen(now->cmd) - 1;
    while(i >= 0 && now->cmd[i] != '&')
		i--;
    now->cmd[i] = '\0';
    
    printf("%s\n", now->cmd);
    tcsetpgrp(0,now->pid);
    kill(now->pid, SIGCONT); //向对象作业发送SIGCONT信号，使其运行
	sleep(1);//等待一秒，保证子进程收到开始启动信号（有没有更好的改法）
	signal(SIGTTOU,SIG_IGN);
	waitpid(fgPid, NULL, 0);//父进程等待前台进程的运行，成功结束返回fgPid
	tcsetpgrp(0,getpid());
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
        printf("pid为7 %d 的作业不存在！\n", pid);
        return;
    }
    //now->isBack=1;//后台运行？
    strcpy(now->state, RUNNING); //修改对象作业的状态
    printf("[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);
    
    kill(now->pid, SIGCONT); //向对象作业发送SIGCONT信号，使其运行
    fgPid=0;
    signal(SIGTTOU,SIG_IGN);
    tcsetpgrp(0,getpid());
}


/*移除一个作业*/
void rmJob(int sig, siginfo_t *sip, void* noused){
    pid_t pid;
    Job *now = NULL, *last = NULL;
    
    if(MY_DEBUG) printf("in rmjobs ingnore -->>%d\n",ingnore);
    

    
    
    pid = sip->si_pid;

    now = head;	
	while(now != NULL && now->pid < pid){
		last = now;
		now = now->next;
	}
    

    if(now == NULL){ //作业不存在，必为初次前台进程暂停或死亡
    	sig_chld();
    	now=addJob(pid);//强制添加
    	int res=kill(now->pid,0);
    	//printf("check res-->%d  pid-->%d\n",res,pid);
    	if(res==0)//被暂停的
   	 	{
			strcpy(now->state, STOPPED); 
			now->cmd[strlen(now->cmd)] = '&';
			now->cmd[strlen(now->cmd) + 1] = '\0';
			printf("[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);
			ingnore=0;
			return;
    	}
    	else//死亡了，删去jobs
    	{
    			if(MY_DEBUG)printf("fg dead!\n");
    			//printf("nowpid:%d  headpid:%d\n",now->pid,head->pid);
    		    if(now == head){
        			head = now->next;
    			}else{
        			last->next = now->next;
    			}
    			free(now);
    			ingnore=0;
    			return;
    	}
    }
    
    //作业存在，必为挂起过的前台进程或后台进程 暂停或死亡
    
    //if(ingnore == 1){
     //   ingnore = 0;
        //sig_chld();//处理僵死
        //return;
    //}
    
    sig_chld();
    int res=kill(now->pid,0);
    if(res==0)//被暂停的
    {
    	sleep(1);
    	strcpy(now->state, STOPPED); 
    	//now->cmd[strlen(now->cmd)] = '&';
    	//now->cmd[strlen(now->cmd) + 1] = '\0';
    	//printf("***[%d]\t%s\t\t%s\n", now->pid, now->state, now->cmd);
    	ingnore=0;
    	return;
    }
    
    if(MY_DEBUG) printf("after fg process\n");
	//开始移除该作业
    if(now == head){
        head = now->next;
    }else{
        last->next = now->next;
    }
    
    //sig_chld();//处理僵死
    free(now);
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
    sigaction(SIGCHLD, &action, NULL);//注意，已经注册信号量
    signal(SIGTSTP, ctrl_Z);
    signal(SIGINT,ctrl_C);
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
    //cmd->args = (char**)malloc(sizeof(char*) * (k + 1));
	cmd->args = (char**)malloc(sizeof(char*) * (k + 1000));//jsz add 为匹配参数增加100个空间
	for(i=0;i<1000;i++)//jsz add
    	cmd->args[i] = NULL;
    for(i = 0; i<k; i++){
        j = strlen(buff[i]);
        cmd->args[i] = (char*)malloc(sizeof(char) * (j + 1));   
        strcpy(cmd->args[i], buff[i]);
		//jsz add 对参数进行匹配操作-----------------------------------------------------------------------------------------------------------
		if(i==0||(strstr(cmd->args[i],"*")==NULL&&strstr(cmd->args[i],"?")==NULL))
			continue;//对命令不进行匹配操作 不包含通配符也不进行匹配操作
		char buf[1000];//目录信息
		//char membuf[1000];//原始目录信息	
		//DIR *dir;
		//struct dirent *ptr;
		char queue[1000][1000]={0};//匹配成功队列
		int qcount=0;	
		char pathandstr[1000];

		getcwd(buf,sizeof(buf));//获取当前目录
		if(strstr(cmd->args[i],"/home")==NULL){//参数不包含绝对路径
			snprintf(pathandstr,1000,"%s/%s",buf,cmd->args[i]);//为cmd->args配上目录
			dirsearch(pathandstr,buf,queue,&qcount,"");//匹配 结果存入queue
		}
		else{//参数包含绝对路径
			strcpy(pathandstr,cmd->args[i]);
			char path[1000]={0};
			char path2[1000]={0};
			char *str1=(char*)malloc(sizeof(cmd->args[i]));
			char *str2;
			revstr(cmd->args[i],str1);
			str2=strstr(str1,"/");
			revstr(str2,path);//path为获取到的路径
			strcpy(path2,path);
			path[strlen(path)-1]='\0';
			dirsearch(pathandstr,path,queue,&qcount,path2);//匹配 结果存入queue
			free(str1);
		}
		//对匹配结果进行排序
		strsort(queue,&qcount);
		if(qcount>0&&cmd->args[i]!=NULL){
			free(cmd->args[i]);//去除原有信息Vim: Caught deadly signal HUP
			int newi;
			//更新参数信息
			for(newi=0;newi<qcount;newi++){
				cmd->args[i+newi]=(char*)malloc(sizeof(char)*(strlen(queue[newi])+1));
				strcpy(cmd->args[i+newi],queue[newi]);
			}			
			//更新i和k
			i=i+qcount-1;
			k=k+qcount-1;
			//清除匹配成功队列
			qcount=0;
			int x;
			int y;
			for(x=0;x<1000;x++)
				for(y=0;y<1000;y++)
					queue[x][y]=0;
		}
		//...---------------------------------------------------------------------------------------------------------------------------------
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
    int pipeIn, pipeOut;
    
    if(exists(cmd->args[0])){ //命令存在

        if((pid = fork()) < 0){
            perror("fork failed");
            return;
        }
        
        if(pid == 0){ //子进程
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
                
                kill(getppid(), SIGUSR1);//子进程设置已完成，即将开始运行
            }
			signal(SIGTTIN,SIG_DFL);
			signal(SIGTTOU,SIG_DFL);
            justArgs(cmd->args[0]);
            setpgid(0,0);//设定一个子进程组,防止被前台挂起命令挂起
            
            if(execv(cmdBuff, cmd->args) < 0){ //执行命令
                printf("execv failed!\n");
                _exit(0);//放弃使用return
            }
        }
		else{ //父进程
            if(cmd ->isBack){ //后台命令             
                fgPid = 0; //pid置0，为下一命令做准备
                addJob(pid); //增加新的作业
                sleep(1);//稍待子进程运行
                kill(pid, SIGUSR1); //子进程发信号，表示作业已加入
                
                signal(SIGUSR1, setGoon); //收到信号，setGoon函数将goon置1，以跳出下面的循环
                while(goon == 0); //等待子进程SIGUSR1信号，表示已开始
                goon = 0; //置0，为下一命令做准备
                
            }else{ //非后台命令
                fgPid = pid;
                signal(SIGTTOU,SIG_IGN);
                ingnore=1;//前台禁止删除作业
                tcsetpgrp(0,pid);
                waitpid(pid,&status,0);
                tcsetpgrp(0,getpid());
                fgPid = 0;//非后台命令返回后需要将前台置为零
            }
		}
    }else{ //命令不存在
        printf("找不到命令 15%s\n", inputBuff);
    }
}

/*执行命令*/
void execSimpleCmd(SimpleCmd *cmd){
    int i, pid;
    char *temp;
    Job *now = NULL;
    
    if(strcmp(cmd->args[0], "exit") == 0) { //exit命令
		for(i = 1, now = head; now != NULL; now = now->next, i++){
                kill(now->pid,SIGKILL);
            }
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
    }else if(strcmp(cmd->args[0],"echo")==0){//echo命令
    	i=1;
    	while(cmd->args[i]!=NULL){
    		if(strstr(cmd->args[i],"\"")==NULL)//不存在引号则原样输出
				printf("%s ",cmd->args[i]);
			else{//存在引号则将引号去除
				char* newstr=(char*)malloc(sizeof(cmd->args[i]));
				int j;
				int len=strlen(cmd->args[i]);
				int newstrcount=0;
				for(j=0;j<len;j++)
					if(cmd->args[i][j]!='\"')
						newstr[newstrcount++]=cmd->args[i][j];
				printf("%s ",newstr);
				free(newstr);
			}		 
			i++;
		}
		printf("\n");
    }else{ //外部命令
        execOuterCmd(cmd);
    }
    
    //释放结构体空间
    free(cmd->input);
    free(cmd->output);
    for(i = 0; cmd->args[i] != NULL; i++){
        free(cmd->args[i]);
    }
}

/*******************************************************
                     命令执行接口
********************************************************/
void execute(){
    //jsz add
	char *delim="|";
	char *inputBuffCopy=(char*)malloc(sizeof(char)*strlen(inputBuff));
	char *part=NULL;
	char *newpart=NULL;
	char *meminputBuff=(char*)malloc(sizeof(char)*strlen(inputBuff));
	int cmdcount=0;//命令计数器
	char myEOF[1];
	
	snprintf(myEOF,10,"%c",EOF);
	if(strstr(inputBuff,myEOF)!=NULL){//去掉EOF
		int i;
		int len=strlen(inputBuff);
		char newstr[1000]={0};
		int newstrcount=0;
		for(i=0;i<len;i++)
			if(inputBuff[i]!=EOF)
				newstr[newstrcount++]=inputBuff[i];
			strcpy(inputBuff,newstr);
	}
	if(strstr(inputBuff,"|")!=NULL){//当输入中包含管道符|
		strcpy(inputBuffCopy,inputBuff);
		strcpy(meminputBuff,inputBuff);
		part=strtok(inputBuffCopy,delim);
		while(part!=NULL){
			newpart=(char*)malloc(sizeof(char)*strlen(inputBuff));
			strcpy(newpart,part);
			part=strtok(NULL,delim);//读取下一个命令
			strcpy(inputBuff,newpart);
			SimpleCmd *cmd = handleSimpleCmdStr(0, strlen(inputBuff));
			if(cmdcount==0){
				cmd->output=(char*)malloc(sizeof(char)*6);
				strcpy(cmd->output,"temp1");
			}
			else if(part!=NULL){
				cmd->input=(char*)malloc(sizeof(char)*6);
				cmd->output=(char*)malloc(sizeof(char)*6);
				if(cmdcount%2==1){
					strcpy(cmd->input,"temp1");
					strcpy(cmd->output,"temp2");
				}
				else{
					strcpy(cmd->input,"temp2");
					strcpy(cmd->output,"temp1");
				}
			}
			else{
				cmd->input=(char*)malloc(sizeof(char)*6);
				if(cmdcount%2==1){
					strcpy(cmd->input,"temp1");
				}
				else{
					strcpy(cmd->input,"temp2");
				}
			}
			cmdcount++;
			execSimpleCmd(cmd);//执行命令
			strcpy(inputBuff,meminputBuff);//将原来的inputBuff还原，以便添加历史记录
			free(newpart);
		}
	}
	else{
    	SimpleCmd *cmd = handleSimpleCmdStr(0, strlen(inputBuff));
		execSimpleCmd(cmd);
	}
	free(inputBuffCopy);
	free(meminputBuff);
	//printf("%d\n",tcgetpgrpt(0));
        //tcsetgrpt();
	//...
}

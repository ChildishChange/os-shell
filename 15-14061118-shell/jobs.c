#include "jobs.h"
#include "global.h"
#include <sys/types.h>
#include <unistd.h>
#include <malloc.h>
#include <string.h>
static Job* list;
//extern char* inputBuff;

typedef struct listInt{
	int pid;
	struct listInt* next;
}*plistInt;

void displayJobs()
{
	int i;
	Job* tmp;
	for(i=1,tmp=list;tmp;tmp=tmp->next,++i)
		printf("[%d]\t%d\t%s\t\t%s\n",i,getpgid(tmp->pids->pid),tmp->state,tmp->cmd);
}

int findJob(pid_t pid,Job** job)
{
	Job* tmp = list;
	int i=0;
	*job = NULL;
	if(!list)
		return 0;
	while(tmp){
		++i;
		plistInt t = tmp->pids;
		while(t && t->pid != pid)
			t = t->next;
		if(t && t->pid == pid){
			if(job)
				*job=tmp;
			return i;
		}
		tmp = tmp->next;
	}
	return 0;
}
int findJobg(pid_t pid,Job** job)
{
	Job* tmp = list;
	int i=0;
	*job = NULL;
	if(!list)
		return 0;
	while(tmp){
		++i;
		if(tmp->pgid == getpgid(pid)){
			*job = tmp;
			return i;
		}
		tmp = tmp->next;
	}
	return 0;

}

void addJob(pid_t pid)
{
	Job* tmp;
	plistInt t;
	findJobg(pid,&tmp);
	if(tmp){
		t = tmp->pids;
		while(t->next && t->pid != pid)
			t = t->next;
		if(t->pid == pid)
			return;
		t->next = (plistInt)malloc(sizeof(listInt));
		//fprintf(stderr,"-----	add job %d	%d\n",tmp->pgid,pid);
		t->next->next=NULL;
		t->next->pid = pid;
	}
	else{
		int i = 1;
		tmp = (Job*)malloc(sizeof(Job));
		//fprintf(stderr,"-----	new job %d	%d\n",getpgid(pid),pid);
		tmp->next = NULL;
		tmp->state = RUNNING;
		tmp->pids = NULL;
		strncpy(tmp->cmd,inputBuff,100);//strlen(inputBuff));
		tmp->pids = (plistInt)malloc(sizeof(listInt));
		tmp->pids->pid = pid;
		tmp->pids->next = NULL;
		if(!list)
			list = tmp;
		else{
			Job* t = list;
			while(t->next){
				++i;
				t = t->next;
			}
			t->next = tmp;
		}
		fprintf(stderr,"[%d]\t%s\t%s",i,tmp->state,tmp->cmd);
	}
	tmp->pgid = getpgid(pid);
	for(t = tmp->pids;t;t=t->next)
		fprintf(stderr,"%d ",t->pid);
	fprintf(stderr,"\n");
}
pid_t rmJob(pid_t pid)
{
	Job* tmp;
	plistInt p;
	int i;
	i = findJob(pid,&tmp);
	if(!tmp)
		return 0;
	p = tmp->pids;
	if(p->pid == pid){
		tmp->pids = p->next;
		//fprintf(stderr,"-----	rm job %d\n",pid);
		free(p);
		
	}
	else{
		while(p->next && p->next->pid != pid)
			p = p->next;
		if(p->next && p->next->pid == pid){
			plistInt t = p->next;
			p->next = t->next;
			free(t);
			//fprintf(stderr,"-----	rm job %d\n",pid);
		}
	}
	if(tmp->pids==NULL){
		Job* t = list;
		pid_t tpp;
		tmp->state = STOPPED;
		tpp = tmp->pgid;
		fprintf(stderr,"[%d]\t%s\t%s",i,tmp->state,tmp->cmd);
		if(list  == tmp)
			list = tmp->next;
		else{
			while(t->next != tmp)
				t = t->next;
			t->next = tmp->next;
		}
		free(tmp);
		//fprintf(stderr,"-----	destroy job %d\n",pid);
		return -tpp;
			
	}
	return tmp->pgid;

}
Job* findJobId(int jid){
	Job* tmp = list;
	while(tmp && --jid)
		tmp=tmp->next;
	return tmp;
}

pid_t getJobpgid(Job* job){
	if(!job)
		return 0;
	return getpgid(job->pids->pid);
}

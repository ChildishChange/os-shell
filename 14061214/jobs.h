#ifndef _JOBS_H
#define _JOBS_H
#include <sys/types.h>
#define STOPPED "stopped"
#define RUNNING "running"
#define DONE    "done"
typedef struct listInt *plistInt;
typedef struct listInt listInt;
typedef struct Job {
        plistInt pids;          //进程号
		pid_t pgid;			//作业进程组号
        char cmd[100];    //命令名
        const char* state;   //作业状态
        struct Job *next; //下一节点指针
    } Job;
void addJob(pid_t pid);
pid_t rmJob(pid_t pid);
void displayJobs();
Job* findJobId(int jid);
pid_t getJobpgid(Job* job);
#endif

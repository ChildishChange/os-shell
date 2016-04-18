#ifndef _global_H
#define _global_H

#ifdef	__cplusplus
extern "C" {
#endif   
    
    #define HISTORY_LEN 10
    
    #define STOPPED "stopped"
    #define RUNNING "running"
    #define DONE    "done"
    #define KILLED "interrupted"
   
    #include <stdio.h>
    #include <stdlib.h>
    #include <pwd.h>
    #include <unistd.h>
    typedef struct asChain {
        char *s;
        struct asChain* next;
    } Argc;
    typedef struct SimpleCmd {
        int isBack,argc;     // �Ƿ��̨����
        char **args;    // �������
        char *input;    // �����ض���
        char *output;   // ����ض���
        struct SimpleCmd* next;
         Argc *ac;
    } SimpleCmd;

    typedef struct History {
        int start;                    //��λ��
        int end;                      //ĩλ��
        char cmds[HISTORY_LEN][100];  //��ʷ����
    } History;

    typedef struct Job {
        int pid;          //���̺�
        char cmd[100];    //������
        char state[10];   //��ҵ״̬
        struct Job *next; //��һ�ڵ�ָ��
    } Job;
     typedef struct PipeNode {
        pid_t pid;
        int writePipe, readPipe, status;
        struct PipeNode* next;
    } pNode;
    char *inputBuff;  //������������
    
    void init();
    void addHistory(char *history);
    void execute();

#ifdef	__cplusplus
}
#endif

#endif	/* _global_H */

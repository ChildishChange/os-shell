#ifndef _global_H
#define _global_H

#ifdef	__cplusplus
extern "C" {
#endif   
    
    #define HISTORY_LEN 10
    
    #define STOPPED "stopped"
    #define RUNNING "running"
    #define DONE    "done"

    #include <stdio.h>
    #include <stdlib.h>
    
    typedef struct SimpleCmd {
        int isBack;     // �Ƿ��̨����
        char **args;    // �������
        char *input;    // �����ض���
        char *output;   // ����ض���
    } SimpleCmd;

    typedef struct ComplexCmd {
        int isBack;
        int num;
        SimpleCmd **cmds;
    } ComplexCmd;
 
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
    
    char inputBuff[100];  //������������

    int len;    

    void init();
    void addHistory(char *history);
    void execute();
    
    static char* builtin[]={".",":","[","alias","bg","bind","break","builtin","caller","cd",
                                "command","compgen","complete","compopt","continue",
                                "declare","dirs","disown","echo","enable","eval","exec","exit",
                                "export","false","fc","fg","getopts","hash","help","history",
                                "jobs","kill","let","local","logout","mapfile","popd","printf",
                                "pushd","pwd","read","readarray","readonly","return","set",
                                "shift","shopt","suorce","suspend","test","times","trap","true",
                                "type","typeset","ulimit","umask","unalias","unset","wait",NULL
                            };

#ifdef	__cplusplus
}
#endif

#endif	/* _global_H */

#ifndef _global_H
#define _global_H
//±êÖ¾ÏÂÃæµÄ´úÂëÊÇcpp´úÂë£¬ÓÃCµÄ·½Ê½È¥·­ÒëÁ´½Ó
#ifdef	__cplusplus
extern "C" {
#endif   
    
    #define HISTORY_LEN 10
    
    #define STOPPED "stopped"
    
    //++++++++++++++++++
    #define KILLED "KILLED"
    //++++++++++++++++++
    
    #define RUNNING "running"
    #define DONE    "done"

    #include <stdio.h>
    #include <stdlib.h>
    
    typedef struct SimpleCmd {
        int isBack;     // ÊÇ·ñºóÌ¨ÔËÐÐ
        char **args;    // ÃüÁî¼°²ÎÊý
        char *input;    // ÊäÈëÖØ¶¨Ïò
        char *output;   // Êä³öÖØ¶¨Ïò
    } SimpleCmd;

    typedef struct History {
        int start;                    //Ê×Î»ÖÃ
        int end;                      //Ä©Î»ÖÃ
        char cmds[HISTORY_LEN][100];  //ÀúÊ·ÃüÁî
    } History;

    typedef struct Job {
        int pid;          //½ø³ÌºÅ
        char cmd[100];    //ÃüÁîÃû
        char state[10];   //×÷Òµ×´Ì¬
        struct Job *next; //ÏÂÒ»½ÚµãÖ¸Õë
    } Job;
    
    char inputBuff[100];  //´æ·ÅÊäÈëµÄÃüÁî
    
    void init();
    void addHistory(char *history);
    void execute();

#ifdef	__cplusplus
}
#endif

#endif	/* _global_H */

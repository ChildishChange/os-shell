#include <stdio.h>
#include <stdlib.h>
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
#include <sys/uio.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/termios.h>

int pid;
int fd[2];

void ctz()
{
	printf("capture a SIGTSTP signal.\n");
}

void mes()
{
	printf("child exited. restart\n");
}

char s[4][100]={"./io","./io","./io","./io"};
char *a[4];
char in[4][100]={"",".",".","."};
char out[5][100]={".",".",".",""};

int main()
{
	int pid,i,pipeIn,pipeOut;
	if (pipe(fd)==-1) printf("%s\n", "create pipe failed.");
	else
	{
		for (i=0;i<4;++i)
		{
			pid=fork();
			if (pid>0)
			{
				printf("%s%d\n","parent waiting child ",pid);
				waitpid(pid,NULL,WUNTRACED);
				printf("%s%d is back.\n","child ",pid);
			}
			else break;
		}
		if (pid>0) return 0;
		printf("FD0: %d, FD1: %d\n",fd[0],fd[1]);
		if (strlen(in[i])>1)
		{
            if((pipeIn = open(in[i], O_RDONLY, S_IRUSR|S_IWUSR)) == -1){
                printf("不能打开文件 %s！\n", in[i]);
                return 0;
            }
		}
		else if (strlen(in[i])==1&&in[i][0]=='.')
		{
			pipeIn=fd[0];			
		}
		else pipeIn=0;
		if (strlen(out[i])>1)
		{
            if((pipeOut = open(out[i], O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1){
                printf("不能打开文件 %s！\n", out[i]);
                return 0;
            }
		}
		else if (strlen(out[i])==1&&out[i][0]=='.')
		{
			pipeOut=fd[1];			
		}
		else pipeOut=1;
		printf("child %d : IN: %d, OUT: %d \n",getpid(),pipeIn,pipeOut);
            if(dup2(pipeIn, 0) == -1){
                printf("重定向标准输入错误！\n");
                return;
            }
            if(dup2(pipeOut, 1) == -1){
                printf("重定向标准输出错误！\n");
                return;
            }
        if(execv(s[i], a) < 0){ //执行命令
            printf("execv failed!\n");
            return;
        }		
	}
}


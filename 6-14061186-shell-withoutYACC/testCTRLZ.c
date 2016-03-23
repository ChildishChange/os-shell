#include <stdio.h>
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

int pid;

void ctz()
{
	printf("capture a SIGTSTP signal.\n");
}

void mes()
{
	printf("child exited. restart\n");
}

int main()
{
	int i=0;
	pid=fork();
	char *argv[4];
	if (pid)
	{
		signal(SIGTSTP,ctz);
		signal(SIGCHLD,mes);
	}
	else 
	{
		signal(SIGTSTP,ctz);
		execv("./Demo",argv);		
	}
	while (i<30)
	{
		sleep(1);
		++i;
		printf(pid?"fa\n":"ch\n");
	}
}

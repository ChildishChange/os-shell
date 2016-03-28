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
	int n;
	scanf("%d",&n);
	--n;
	while (n-->0) printf("%d\n",n);
}


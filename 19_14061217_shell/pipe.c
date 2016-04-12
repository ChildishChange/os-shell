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
int pipe_fd[2];

void ctz()
{
	printf("capture a SIGTSTP signal.\n");
}

void mes()
{
	printf("child exited. restart\n");
}

char s[4][100]={"./io","./io","./io","./io"};
char *a[4]
char in[4][100] = {"",".",".","."};
char out[4][100] = {".",".",".",""}


int main() {

	int pid, pipe_in, pipe_out;
	int i;

	if ( pipe(pipe_fd) == -1 ) {
		printf("Pipe create failed\n");
	}

	else {

		for ( i = 0; i < 4; i++ ) {
			pid = fork();

			if ( pid > 0 ) {
				printf("parent process is waiting child %d\n", pid );
				waitpid(pid, NULL, WUNTRACED);
				printf("child %d goes back\n", pid );
			}
			else{
				break;
			}
		}

		if ( pid > 0 ) {
			return 0;
		}

		printf("FD0:%d,FD1:%d\n", pipe_fd[0], pipe_fd[1] );

		if ( strlen(in[i]) > 1 ) {
			if( (pipe_in = open(in[i],O_WRONGLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1) {
				printf("can't open the file\n");
				return 0;
			}
		}
		else if ( strlen(in[i]) == 1 && in[i][0] = '.' ) {
			pipe_in = pipe_fd[0];
		}
		else {
			pipe_in = 0;
		}

		if ( strlen(out[i]) > 1 ) {
			if( (pipe_out = open(in[i],O_WRONGLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR)) == -1) {
				printf("can't open the file\n");
				return 0;
			}
		}
		else if ( strlen(out[i]) == 1 && out[i][0] = '.' ) {
			pipe_out = pipe_fd[1];
		}
		else {
			pipe_out = 1;
		}

		printf("child %d: IN: %d, OUT:%d\n", getpid(), pipe_in, pipe_out );

		if ( dup2(pipe_in,0) == -1 ) {
			printf("重定向标准输入错误！\n");
            return;
		}
		if ( dup2(pipe_out,1) == -1 ) {
			printf("重定向标准输chu错误！\n");
            return;
		}

		if( execv(s[i],a) < 0 ) {
			printf("execv failed\n");
			return;
		}

	}
}
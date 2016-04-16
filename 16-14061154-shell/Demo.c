#include <stdio.h>
#include <unistd.h>
#include <signal.h>

int ctrlz()
{
	printf("ctrl_z ,pid:%d\n",getpid());
}
int main(){
	int i = 0;
	printf("Demo is running\n");
	while(i<8){
		i=i+1;
		sleep(1);
		printf("Demo has running %d seconds.\tPGRP:%d\n", i,getpgrp());
		fflush(stdout);
	}
	printf("Demo is ending.\n");
	
}

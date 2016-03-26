#include <stdio.h>

int main(){
	int i = 0;
	int k;
	printf("Demo is running\n");
	while(i<5){
		i=i+1;
		sleep(1);
		printf("Demo has running %d seconds.\n", i);
		fflush(stdout);
	}
}

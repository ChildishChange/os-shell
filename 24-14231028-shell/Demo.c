#include <stdio.h>

int main(){
	int i = 0;
        char d;
	scanf("%c",&d);
	while(i<10){
		i=i+1;
		sleep(1);
		printf("%c\n", d);
		fflush(stdout);
	}
	printf("Demo is ending.\n");
	
}

#include <stdio.h>
int main(){
	int i;
	i=fork();
	if(i){
		wait();
		printf("parent\n");
	}
	else{
		printf("child\n");
	}
	return 0;
}

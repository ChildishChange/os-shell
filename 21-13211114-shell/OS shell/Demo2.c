#include <stdio.h>
#include <string.h>
#include <stdlib.h>

char str [100];

int main(){
	while(1){
		scanf("%s", str);
		printf("%s\n", str);
		memset(str, ' ', sizeof(str));
	}
	return 0;
}

#include <string.h>
#include <stdio.h>
int main(){
	char s1[10];
	char s2[10];
	scanf("%s",s1);
	scanf("%s",s2);
	if(!strcmp(s1,"child")){
		printf("hic\n");
	}
	else{
		printf("hey%s\n",s1);
	}
	if(!strcmp(s2,"parent")){
		printf("hip\n");
	}
	else{
		printf("hey%s\n",s2);
	}
	return 0;
}

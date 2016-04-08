#include <stdio.h>
int main(){
	char s1[10];
	char s2[10];
	scanf("%s",s1);
	scanf("%s",s2);
	if(!strcmp(s1,"child")){
		printf("1atesthic\n");
	}
	else if(!strcmp(s1,"hic")){
		printf("1heyheyhiheyc%s\n",s1);
	}
	else if(!strcmp(s1,"parent")){
		printf("1atesthip\n");
	}
	else if(!strcmp(s1,"hip")){
		printf("1heyheyhiheyp%s\n",s1);
	}
	else{
		printf("1third%s\n",s1);
	}
	if(!strcmp(s2,"child")){
		printf("2atesthic\n");
	}
	else if(!strcmp(s2,"hic")){
		printf("2heyheyhiheyc%s\n",s2);
	}
	else if(!strcmp(s2,"parent")){
		printf("2atesthip\n");
	}
	else if(!strcmp(s2,"hip")){
		printf("2heyheyhiheyp%s\n",s2);
	}
	else{
		printf("2third%s\n",s2);
	}
	return 0;
}

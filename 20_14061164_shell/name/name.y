%{
typedef char* string;
#define YYSTYPE string
%}
%token NAME EQ AGE BQ CQ
%%
file :record file | record;
record : NAME EQ AGE NAME NAME AGE BQ AGE EQ AGE{
	printf("\n%s is %s years old!!!\n",$1,$3);
	 int i=0;
	 int j=0;
	 int l=0;
	 int k=0;
 	 int m=0;	
	if((strcmp($4,"yty"))==0&&(strcmp($5,"os"))==0){
		i=atoi($6);j=atoi($8);l=atoi($10);
		if($7[0]=='+'&&strlen($7)==1) {
			//printf("+\n");
			k=i+j; if(k==l) printf("yes you are right\n");
					else {printf("no you fail\n");
						printf("%d + %d = %d \n",i,j,k);}
		}
		else if($7[0]=='-'){
			//printf("-\n");
			k=i-j; if(k==l) printf("yes you are right\n");
					else {printf("no you fail\n");
						printf("%d - %d = %d \n",i,j,k);}
		}else if($7[0]=='+'&&$7[1]=='+'){
			//printf("+\n");
			k=i*j; if(k==l) printf("yes you are right\n");
					else {printf("no you fail\n");
						printf("%d ++ %d = %d \n",i,j,k);}
		}else if($7[0]=='/'){
			//printf("+\n");
			k=i/j; if(k==l) printf("yes you are right\n");
					else {printf("no you fail\n");
						printf("%d / %d = %d \n",i,j,k);}
		}
	} 
         else {printf("your account or password is wrong\n");}
	 //printf("%s %s %s\n",$4,$5,$6);
	
            //if($4 == "++" ) {printf("is $\n");}
            //else {printf("is not $\n");}
        //if($5 == "--" ) {printf("is %\n");}             
        //else {printf("is not %\n");}
};
%%
int main(){
yyparse();
return 0;
}
int yyerror(char *msg){
printf("Error encountered : %s\n",msg);
}

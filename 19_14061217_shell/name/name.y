%{
typedef char* string;
#define YYSTYPE string
%}
%token NAME EQ AGE SEX
%%
file : record file | record;
record : NAME EQ AGE SEX {printf("%s is %s years old, sex:", $1, $3, $4); 
			if( strcmp($4,",") == 0) printf("M\n");
			else printf("W\n");};
%%
int main() {
yyparse();
return 0;
}
int yyerror(char *msg) {
printf("Error encountered: %s \n", msg);
}

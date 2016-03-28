atea-sh : bison.tab.c execute.c lex.yy.c
	gcc -o atea-sh bison.tab.c execute.c lex.yy.c -g
bison.tab.c:
	bison -d bison.y
lex.yy.c : bison.tab.h
	flex lex.l
clean :
	rm atea-sh bison.tab.c lex.yy.c bison.tab.h

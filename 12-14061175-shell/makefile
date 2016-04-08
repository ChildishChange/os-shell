user-sh : bison.tab.o execute.o
	cc -g -o user-sh bison.tab.o lex.yy.o execute.o -lfl
bison.tab.o : bison global.h
	cc -g -c bison.tab.c lex.yy.c -ggdb
execute.o : execute.c global.h
	cc -g -c execute.c -ggdb
bison : lex.l bison.y
	bison -d bison.y
	flex lex.l
clean :
	rm user-sh bison.tab.o execute.o bison.tab.h lex.yy.c lex.yy.o bison.tab.c

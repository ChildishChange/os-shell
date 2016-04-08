user-sh : bison.tab.o execute.o
	cc -o user-sh bison.tab.o execute.o lex.yy.o

bison.tab.o : bison.tab.c global.h lex.yy.c bison.tab.h
	cc -c bison.tab.c lex.yy.c

execute.o : execute.c global.h
	cc -c execute.c

bison.tab.c:
	bison -d bison.y

lex.yy.c : 
	lex bison.lex



clean :
	rm user-sh bison.tab.o execute.o bison.tab.c lex.yy.c  lex.yy.o 

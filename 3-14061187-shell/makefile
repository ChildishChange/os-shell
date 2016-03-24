user-sh : lex.yy.o bison.tab.o execute.o
	cc -o user-sh bison.tab.o execute.o lex.yy.o 

bison.tab.o : bison.tab.c global.h
	cc -c bison.tab.c

lex.yy.o: bison.tab.c global.h lex.yy.c
	cc -c lex.yy.c

execute.o : execute.c global.h
	cc -c execute.c

bison.tab.c bison.tab.h: bison.y
	bison -d bison.y

lex.yy.c : lex.l
	flex lex.l

clean :
	rm user-sh bison.tab.o execute.o bison.tab.c bison.tab.h

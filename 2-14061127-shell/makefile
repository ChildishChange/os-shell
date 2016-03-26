user-sh : bison.tab.o execute.o lex.yy.o
	cc -o user-sh bison.tab.o execute.o lex.yy.o
bison.tab.o : bison.tab.c global.h lex.yy.c
	cc -c bison.tab.c lex.yy.c
execute.o : execute.c global.h lex.yy.c
	cc -c execute.c
lex.yy.o : lex.yy.c global.h
	cc -c lex.yy.c
bison.tab.c bison.tab.h : bison.y global.h lex.yy.c
	bison -d bison.y
lex.yy.c: bison.lex global.h
	lex bison.lex
clean :
	rm user-sh bison.tab.o execute.o bison.tab.c lex.yy.c lex.yy.o bison.tab.h

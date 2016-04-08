our-shell	: bison.tab.o execute.o lex.yy.o
	cc -o our-shell bison.tab.o execute.o lex.yy.o
bison.tab.o 	: bison.tab.c
	cc -c bison.tab.c
lex.yy.o	: lex.yy.c bison.tab.h
	cc -c lex.yy.c
execute.o 	: execute.c global.h
	cc -c execute.c
lex.yy.c 	: bison.tab.h
	flex flex.lex
bison.tab.c bison.tab.h:
	bison -d bison.y
clean 	:
	rm our-shell bison.tab.o execute.o bison.tab.c bison.tab.h lex.yy.c lex.yy.o

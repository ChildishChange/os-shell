user-sh : bison.tab.o execute.o lex.yy.o
	cc -o user-sh bison.tab.o execute.o lex.yy.o
bison.tab.o : bison.tab.c global.h
	cc -c bison.tab.c
lex.yy.o : lex.yy.c global.h
	cc -c lex.yy.c
execute.o : execute.c global.h
	cc -c execute.c
lex.yy.c : bison.tab.c global.h
	lex lex.l
bison.tab.c:
	bison -d bison.y
clean :
	rm user-sh bison.tab.o execute.o bison.tab.c lex.yy.o lex.yy.c bison.tab.h
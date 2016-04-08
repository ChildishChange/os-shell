user-sh : bison.tab.o execute.o bison.tab.h
	cc -o user-sh bison.tab.o execute.o lex.yy.o 
bison.tab.o : bison.tab.c global.h bison.tab.h lex.yy.c
	cc -c bison.tab.c lex.yy.c
execute.o : execute.c global.h
	cc -c execute.c
bison.tab.c:bison.y global.h
	bison -d bison.y
lex.yy.c:bison.tab.h name.lex global.h
	lex name.lex
clean :
	rm user-sh bison.tab.o execute.o bison.tab.c lex.yy.c lex.yy.o bison.tab.h

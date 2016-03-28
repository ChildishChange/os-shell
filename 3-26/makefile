user-sh : lex.yy.o y.tab.o execute.o
	cc -g -o user-sh y.tab.o execute.o lex.yy.o
lex.yy.o: lex.yy.c y.tab.c global.h
	cc -g -c lex.yy.c
y.tab.o : y.tab.c global.h
	cc -g -c y.tab.c
execute.o : execute.c global.h
	cc -g -c execute.c
y.tab.c y.tab.h:
	yacc -d yacc.y
lex.yy.c:
	lex lextest.l
clean :
	rm user-sh *.o lex.yy.c y.tab.h y.tab.c

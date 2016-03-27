CC = cc -g
user-sh : lex.yy.o bison.tab.o execute.o
	$(CC) -o user-sh bison.tab.o execute.o lex.yy.o
	echo stQ_libo_Orz

bison.tab.o : bison.tab.c global.h
	$(CC) -c bison.tab.c

lex.yy.o: bison.tab.c global.h lex.yy.c
	$(CC) -c lex.yy.c

execute.o : execute.c global.h
	$(CC) -c execute.c

bison.tab.c bison.tab.h: bison.y
	bison -d bison.y

lex.yy.c : lex.l
	flex lex.l

clean :
	rm user-sh *.o lex.yy.c bison.tab.h bison.tab.c


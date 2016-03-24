CFLAGS+=-g
user-sh : bison.o execute.o jobs.o lex.yy.o
	$(CC) $(CFLAGS) -o $@ $^

bison.c: bison.y
	bison -d bison.y
	mv -f bison.tab.c bison.c
	mv -f bison.tab.h bison.h

lex.yy.o: lex.yy.c

lex.yy.c: lex.l
	lex lex.l
clean :
	rm *.o  user-sh bison.c bison.h lex.yy.c
end :
	@kill -9 `ps -C user-sh -C Demo -o pid=` 2>/dev/null

dis :
	@ps -o ppid -o pid -o pgrp -o cmd -a

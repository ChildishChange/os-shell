cal:lex.yy.c cal.tab.c cal.tab.h
	gcc lex.yy.c cal.tab.c -o cal
lex.yy.c:cal.lex cal.tab.h
	lex cal.lex
cal.tab.c:cal.y
	bison -d cal.y
cal.tab.h:cal.y
clean:
	rm *c *h cal

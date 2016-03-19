user-sh : bison.tab.o execute.o
	cc -o user-sh bison.tab.o execute.o
bison.tab.o : bison.tab.c global.h
	cc -c bison.tab.c -Wformat=0
execute.o : execute.c global.h
	cc -c execute.c -Wformat=0
bison.tab.c:
	bison bison.y
clean :
	rm user-sh bison.tab.o execute.o bison.tab.c

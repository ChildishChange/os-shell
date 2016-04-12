objects = bison.tab.o execute.o

user-sh : $(objects)
	cc -o user-sh $(objects)
bison.tab.o : bison.tab.c global.h
	cc -c bison.tab.c
execute.o : execute.c global.h
	cc -c execute.c
bison.tab.c:
	bison bison.y

#celan的规则是删除目标文件（.o和执行文件），
#这里为什么删除了c文件
.PHONY: clean
clean :
	rm user-sh bison.tab.o execute.o bison.tab.c

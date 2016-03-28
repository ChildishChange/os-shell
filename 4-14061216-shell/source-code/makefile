# 最终生成的可执行文件user-sh  必须放在首行。 使用./user-sh  命令运行，然后就可以进行测试了
user-sh : bison.tab.o execute.o lex.yy.o
	cc -o user-sh bison.tab.o execute.o lex.yy.o
lex.yy.o: global.h lex.yy.c bison.tab.h
	cc -c -g lex.yy.c 
bison.tab.o : bison.tab.c global.h lex.yy.c bison.tab.h
	cc -c -g bison.tab.c lex.yy.c
execute.o : execute.c global.h
	cc -c -g execute.c 
#这里不写依赖文件是利用了自动补全机制？
bison.tab.c:
	bison -d bison.y

lex.yy.c:
	lex bison.lex
#防重名
.PHONY:clean                           
 #伪目标，使用make命令编译时并不会执行，要使用make clean命令。用于清理之前编译的目标文件
clean :                               
	-rm user-sh bison.tab.o execute.o bison.tab.c lex.yy.o lex.yy.c # - 避免因某个文件有问题导致整个命令都无法执行的情况

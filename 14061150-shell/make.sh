bison -d bison.y
flex flex.lex
gcc execute.c bison.tab.c lex.yy.c -o our-shell

user-sh : bison.o execute.o jobs.o
	$(CC) $(CFLAGS) -o $@ $^
clean :
	rm *.o  user-sh
.PHONY: end
end :
	@kill -9 `ps -C user-sh -C Demo -o pid=` 2>/dev/null

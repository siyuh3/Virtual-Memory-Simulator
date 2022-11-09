pmmalloc:
	gcc main.c pmmalloc.c -o pmmalloc -lm
clean:
	rm -f pmmalloc
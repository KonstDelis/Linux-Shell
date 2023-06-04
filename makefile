CC = gcc

all: shell.c 
	$(CC) $^ -o shell

clean: 
	rm -f shell
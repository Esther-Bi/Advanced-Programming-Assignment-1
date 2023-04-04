CC = gcc
FLAGS = -lreadline

all: myshell

run: myshell
	./myshell

myshell: shell.c
	$(CC) -o myshell shell.c $(FLAGS)

clean:
	rm -f *.o myshell
all : myshell

myshell : LineParser.o myshell.o
	gcc -m32 -g -Wall -o myshell myshell.o LineParser.o

LineParser.o: LineParser.c
	gcc -m32 -g -Wall -c -o LineParser.o LineParser.c
	
myshell.o: myshell.c
	gcc -m32 -g -Wall -c -o myshell.o myshell.c

clean :
	rm -f *.o myshell
all : myshell looper

myshell : LineParser.o myshell.o
	gcc -m32 -g -Wall -o myshell myshell.o LineParser.o

looper : looper.o
	gcc -m32 -g -Wall -o looper looper.o

LineParser.o: LineParser.c
	gcc -m32 -g -Wall -c -o LineParser.o LineParser.c
	
myshell.o: myshell.c
	gcc -m32 -g -Wall -c -o myshell.o myshell.c

looper.o: looper.c
	gcc -m32 -g -Wall -c -o looper.o looper.c
	gcc -m32 -g -Wall -o looper looper.o

clean :
	rm -f *.o myshell
	rm -f *.o looper
all : mypipeline myshell looper

mypipeline : mypipeline.o
	gcc -m32 -g -Wall -o mypipeline mypipeline.o

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

mypipeline.o: mypipeline.c
	gcc -m32 -g -Wall -c -o mypipeline.o mypipeline.c
	

clean :
	rm -f *.o myshell
	rm -f *.o mypipeline
	rm -f *.o looper
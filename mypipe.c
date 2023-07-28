#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#define MSGSIZE 5

int main(int argc, char **argv){ 
    //help from https://www.geeksforgeeks.org/pipe-system-call/
    char inbuf[MSGSIZE] = {'\0'};
    int p[2];
    char* msg = "hello";
    if (pipe(p) < 0)
        exit(1);
    
    pid_t pid = fork();
    if (pid == 0) {
        close(p[0]);
        write(p[1], msg, MSGSIZE);
        close(p[1]);
        _exit(1);
    }
    
    else{
        close(p[1]);
        read(p[0], inbuf, MSGSIZE);
        printf("%s\n", inbuf);
        close(p[0]);
        _exit(1);
    }
}
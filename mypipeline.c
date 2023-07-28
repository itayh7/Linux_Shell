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
    int p[2];
    if (pipe(p) < 0)
        exit(1);
    
    fprintf(stderr,"(parent_process>forking…)\n");
    pid_t pid = fork();
    fprintf(stderr,"(parent_process>created process with id: %ld)\n", (long)getpid());
    if (pid == 0) { //child 1
        fprintf(stderr,"(child1>redirecting stdout to the write end of the pipe…)\n");
        close(STDOUT_FILENO);
        dup(p[1]);
        close(p[1]);
        char* const ls_args[] = {"ls", "-1", NULL};
        fprintf(stderr,"(child1>going to execute cmd: %s)\n", ls_args[0]);
        execvp(ls_args[0], ls_args);
        perror("Error!");
        _exit(1);
    }
    
    else{ //parent
        fprintf(stderr, "(parent_process>closing the write end of the pipe…)\n");
        close(p[1]);
        pid_t pid2 = fork();
        int status;
        if (pid2 == 0) { //child 2
            fprintf(stderr, "(child2>redirecting stdin to the read end of the pipe…)\n");
            close(STDIN_FILENO);
            dup(p[0]);
            close(p[0]);
            char* const tail_args[] = {"tail", "-n", "2", NULL};
            fprintf(stderr, "(child2>going to execute cmd %s)\n", tail_args[0]);
            execvp(tail_args[0], tail_args);
            perror("Error!");
            _exit(1);
        }
        else{ //parent
            fprintf(stderr, "(parent_process>closing the read end of the pipe…)\n");
            close(p[0]);
            fprintf(stderr, "(parent_process>waiting for child processes to terminate…)\n");
            waitpid(pid, &status, WCONTINUED);//help from https://www.ibm.com/docs/en/zos/2.3.0?topic=functions-waitpid-wait-specific-child-process-end
            waitpid(pid2, &status, WCONTINUED);
            fprintf(stderr, "(parent_process>exiting…)\n");
        }

    }
}
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>

#include "LineParser.h"

#define input_size 2048
#define quit_command "quit"

#define TERMINATED  -1
#define RUNNING 1
#define SUSPENDED 0
#define HISTLEN 20

typedef struct process{
    cmdLine* cmd;                         /* the parsed command line*/
    pid_t pid; 		                      /* the process id that is running the command*/
    int status;                           /* status of the process: RUNNING/SUSPENDED/TERMINATED */
    struct process *next;	              /* next process in chain */
} process;

process* process_list = NULL;

void closeProcess(process** process_list, pid_t pid){
    if((*process_list)->pid == pid){
        
        process* temp = *process_list;
        
        freeCmdLines((*process_list)->cmd);
        (*process_list) = (*process_list)->next;
        free(temp);
    }
    else{
        
        process* temp = *process_list;
        while(temp->next != NULL && temp->next->pid != pid)
            temp = temp->next;
        if(temp->next != NULL){
            freeCmdLines(temp->next->cmd);
            free(temp->next);
            temp->next = temp->next->next;

        }
    }
}

void freeProcessList(process* process_list){
    while(process_list != NULL)
        closeProcess(&process_list, process_list-> pid);
}

void updateProcessStatus(process* process_list, int pid, int status){
    process* temp = process_list;
    while(temp != NULL && temp->pid != pid)
        temp = temp->next;
    
    if(temp != NULL){
        temp->status = status;
    }

}

void updateProcessList(process **process_list){
    process* temp = *process_list;
    while(temp != NULL){
        int st;
        pid_t status = waitpid(temp->pid, &st, WNOHANG);
        //help from https://www.ibm.com/docs/en/zos/2.3.0?topic=functions-waitpid-wait-specific-child-process-end
        if(temp->status == SUSPENDED)
            updateProcessStatus(*process_list, temp->pid, SUSPENDED);
        else if(status == 0)
            updateProcessStatus(*process_list, temp->pid, RUNNING);
        else if(status == -1)
            updateProcessStatus(*process_list, temp->pid, TERMINATED);
        else if (WIFEXITED(status)) 
            updateProcessStatus(*process_list, temp->pid, TERMINATED);
        else if (WIFSIGNALED(status)) 
            updateProcessStatus(*process_list, temp->pid, TERMINATED);
        else if (WIFSTOPPED(status))
            updateProcessStatus(*process_list, temp->pid, SUSPENDED);
        else if (WIFCONTINUED(status))
            updateProcessStatus(*process_list, temp->pid, RUNNING);
        
        temp = temp->next;
    }
}

void addProcess(process** process_list, cmdLine* cmd, pid_t pid){
    process* new_process = calloc(1, sizeof(process));
    new_process->cmd = cmd;
    new_process->pid = pid;
    new_process->status = RUNNING;
    new_process->next = *process_list;
    *process_list = new_process;
}

void printProcessList(process** process_list){
    updateProcessList(process_list);
    unsigned i = 0;
    process* temp = *process_list;
    char* status[] = {"Terminated", "Suspended", "Running"};
    printf("INDEX   PID     STATUS      COMMAND\n");
    while(temp != NULL){
        printf("%d      %d      %s      ", i++, temp->pid, status[temp->status+1]);
        for(unsigned j = 0; j<temp->cmd->argCount; j++)
            printf(" %s", temp->cmd->arguments[j]);
        
        printf("\n");
        int to_delete_status = temp->status;
        pid_t to_delete = temp->pid;
        temp = temp->next;
        if(to_delete_status == TERMINATED)
            closeProcess(process_list, to_delete);
        
    }
}

void execute_pipe(cmdLine *pCmdLine){
    //help from https://www.geeksforgeeks.org/pipe-system-call/
    int p[2];
    if (pipe(p) < 0)
        exit(1);
    
    pid_t pid = fork();
    addProcess(&process_list, pCmdLine, pid);
    if (pid == 0) { //child 1 - writer
        if(pCmdLine->inputRedirect != NULL){
            FILE* input = fopen(pCmdLine->inputRedirect, "r");
            if(input == NULL){
                perror("Can't open file!");
                _exit(1);
            }
            dup2(fileno(input), 0);
            fclose(input);
        }
        close(STDOUT_FILENO);
        dup(p[1]);
        close(p[1]);
        execvp(pCmdLine->arguments[0], pCmdLine->arguments);
        perror("Error!");
        _exit(1);
    }
    
    else{ //parent
        close(p[1]);
        pid_t pid2 = fork();
        addProcess(&process_list, pCmdLine->next, pid2);
        int status;
        if (pid2 == 0) { //child 2 - reader
            if(pCmdLine->next->outputRedirect != NULL){
                FILE* output = fopen(pCmdLine->next->outputRedirect, "w");
                dup2(fileno(output), 1);
                fclose(output);
            }
            close(STDIN_FILENO);
            dup(p[0]);
            close(p[0]);
            execvp(pCmdLine->next->arguments[0], pCmdLine->next->arguments);
            perror("Error!");
            _exit(1);
        }
        else{ //parent
            close(p[0]);
            waitpid(pid, &status, WCONTINUED);//help from https://www.ibm.com/docs/en/zos/2.3.0?topic=functions-waitpid-wait-specific-child-process-end
            waitpid(pid2, &status, WCONTINUED);
        }

    }
    pCmdLine->next = NULL;
}
void execute(cmdLine *pCmdLine){
    if(strcmp(pCmdLine->arguments[0], "cd") == 0){
        if(chdir(pCmdLine->arguments[1]) == -1)
            perror("Error!");
        freeCmdLines(pCmdLine);
    }
    else if(strcmp(pCmdLine->arguments[0], "procs") == 0){
        printProcessList(&process_list);
        freeCmdLines(pCmdLine);
    }
    else if(strcmp(pCmdLine->arguments[0], "kill") == 0){
        if(kill(atoi(pCmdLine->arguments[1]), SIGINT) == 0)
            printf("%d killed successfully\n", atoi(pCmdLine->arguments[1]));
        freeCmdLines(pCmdLine);
    }
    else if(strcmp(pCmdLine->arguments[0], "suspend") == 0){
        if(kill(atoi(pCmdLine->arguments[1]), SIGTSTP) == 0){
            updateProcessStatus(process_list, atoi(pCmdLine->arguments[1]), SUSPENDED);
            printf("%d suspended successfully\n", atoi(pCmdLine->arguments[1]));
        }
        freeCmdLines(pCmdLine);
    }
    else if(strcmp(pCmdLine->arguments[0], "wake") == 0){
        if(kill(atoi(pCmdLine->arguments[1]), SIGCONT) == 0){
            updateProcessStatus(process_list, atoi(pCmdLine->arguments[1]), RUNNING);
            printf("%d woke up successfully\n", atoi(pCmdLine->arguments[1]));
        }
        freeCmdLines(pCmdLine);
    }
    else if(pCmdLine->next != NULL){ //if there is pipe command
        if(pCmdLine->outputRedirect != NULL || pCmdLine->next->inputRedirect != NULL)
            fprintf(stderr, "Error: you can't do output / input redirection\n");
        else{
            execute_pipe(pCmdLine);
        }
    }
    else{
        pid_t pid = fork();
        addProcess(&process_list, pCmdLine, pid);
        int status;
        if(pid == 0){
            if(pCmdLine->inputRedirect != NULL){
                FILE* input = fopen(pCmdLine->inputRedirect, "r");
                if(input == NULL){
                    perror("Can't open file!");
                    _exit(1);
                }
                dup2(fileno(input), 0);
                fclose(input);
            }
            if(pCmdLine->outputRedirect != NULL){
                FILE* output = fopen(pCmdLine->outputRedirect, "w");
                dup2(fileno(output), 1);
                fclose(output);
            }
            execvp(pCmdLine->arguments[0], pCmdLine->arguments);
            perror("Error!");
            _exit(1);
        }
        else if(pCmdLine->blocking)
            waitpid(pid, &status, WCONTINUED); //help from https://www.ibm.com/docs/en/zos/2.3.0?topic=functions-waitpid-wait-specific-child-process-end
    }
}

int main(int argc, char **argv){ 
    
    int debug = 0;
    for(unsigned i = 1; i<argc; i++)
        if(strcmp(argv[i], "-d") == 0 || strcmp(argv[i], "-D") == 0)
            debug = 1;
    

    char path[PATH_MAX];
    char user_input[input_size];
    
    char* history[HISTLEN] = {'\0'};
    int cmd_count = 0;
    int newest = 0;
    int oldest = 0;
    while(1){
        getcwd(path, PATH_MAX);
        printf("%s: ", path);
        fgets(user_input, input_size, stdin);

        if(user_input[0] == '\n')
            continue;
        if(debug)
            fprintf(stderr, "%s", user_input);
        

        if(user_input[0] == '!'){
            if(strcmp(user_input, "!!\n") == 0)
                strcpy(user_input, history[(newest)%HISTLEN]);
            else{
                int num;
                if(sscanf(user_input, "!%d", &num) != 1 || num < 1 || num > HISTLEN || history[num-1] == NULL){
                    fprintf(stdout, "Error: invalid input\n");
                    continue;
                }
                strcpy(user_input, history[num-1]);
            }
        }
            
        
        char* to_add = malloc(strlen(user_input)+1);
        strcpy(to_add, user_input);
        if(history[(cmd_count)%HISTLEN] != NULL){
            oldest = (cmd_count+1)%HISTLEN;
            free(history[(cmd_count)%HISTLEN]);
        }
        history[(cmd_count)%HISTLEN] = to_add;
        newest = cmd_count++;


        cmdLine* cmd_line = parseCmdLines(user_input);
        int first_arg_size = strlen(cmd_line->arguments[0]);
        if(first_arg_size == strlen(quit_command) && memcmp(cmd_line->arguments[0], quit_command, first_arg_size) == 0){
            freeCmdLines(cmd_line);
            break;
        }
        if(strcmp(user_input, "history\n") == 0){
            for(unsigned i = 0; i<HISTLEN && history[(oldest+i)%HISTLEN] != NULL; i++)
                printf("%d %s", i+1, history[(oldest+i)%HISTLEN]);
            freeCmdLines(cmd_line);
        }
        else
            execute(cmd_line);
    }
    for(unsigned i = 0; i<HISTLEN; i++)
        if(history[i] != NULL)
            free(history[i]);
    freeProcessList(process_list);
}
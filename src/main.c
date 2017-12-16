#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <string.h>
#include "sfish.h"
#include "debug.h"


void runExec(char* input, char* token, char* dir);
void doCD(char* token, char* dir, char* prevDir);
void printProcesses();
int job = 0;
pid_t getProcess(int jid);
int removeProcessJID(int jid);
pid_t childPID;
char* getProcessName(int jid);
int removeProcessPID(pid_t delPid);
void sigIntHandler(int sig){
    if(sig == SIGTSTP){
        char *jobName = getProcessName(job);
        printf("[%d] %s\n", job, jobName);
    }
    if(sig == SIGINT){
        removeProcessJID(job);
        job = 0;
    }
}
void emptyHandler(int sig){
}
int main(int argc, char *argv[], char* envp[]) {
    char* input;
    bool exited = false;

    if(!isatty(STDIN_FILENO)) {
        // If your shell is reading from a piped file
        // Don't have readline write anything to that file.
        // Such as the prompt or "user input"
        if((rl_outstream = fopen("/dev/null", "w")) == NULL){
            perror("Failed trying to open DEVNULL");
            exit(EXIT_FAILURE);
        }
    }
    char* home = getenv("HOME");
    int homeLen = strlen(home);
    char prevDir[1024]; //this is the previous directory
    getcwd(prevDir, sizeof(prevDir));
    do {
        signal(SIGINT, emptyHandler);
        signal(SIGTSTP, emptyHandler);
        signal(SIGTTIN, emptyHandler);
        char dir[1024];//this is the absolute directory
        getcwd(dir, sizeof(dir));

        //if home is in the directory, we use relative dir. Else use absolute dir
        if(memcmp(dir, home, strlen(home)) == 0){
            int currSize = strlen(dir);
            dir[0] = '~';
            memcpy(dir + 1, dir+homeLen, currSize - homeLen);
            memset(dir + currSize - homeLen + 1, 0, 1024);
        }
        // printf("HELLO WORLD\e[s\n");
        // printf("%*s", 50, "bye");
        // printf("\e[u");
        char* name = " :: nabichandani ";
        strcat(dir, name);
        strcat(dir, ">> ");
        input = readline(dir);

        if(input == NULL){
            printf("\n");
            continue;
        }

        int inputlen = strlen(input);
        char inputcpy[inputlen];
        char* delim = " \t";
        strcpy(inputcpy, input);
        char *token = strtok(inputcpy, delim);

        //BUILTIN COMMANDS
        if(input == NULL || inputlen == 0  || token == NULL) {
            continue;
        }
        else if(strcmp(token, "exit") == 0){
            exit(EXIT_SUCCESS);
            continue;
        }
        else if(strcmp(token, "cd") == 0){
            doCD(token, dir, prevDir);
        }
        else if(strcmp(token, "jobs") == 0){
            printProcesses();
        }
        else if(strcmp(token, "fg") == 0){
            token = strtok(NULL, delim);
            if(*token != '%'){
                printf("sfish builtin error: %s\n", "no %");
                continue;
            }
            token = (token + 1);
            int jid = atoi(token);
            if(jid == 0){
                printf("sfish builtin error: %s\n", "invalid job");
                continue;
            }
            pid_t tempPid = getProcess(jid);
            if(tempPid == -1){
                printf("sfish builtin error: %s\n", "invalid job");
                continue;
            }
            job = jid;
            int status;
            signal(SIGCHLD, sigIntHandler);
            tcsetpgrp(0, tempPid);
            kill(tempPid, SIGCONT);
            signal(SIGINT, sigIntHandler);
            waitpid(tempPid, &status, WUNTRACED);
            if((WIFEXITED(status) != 0)){
                sigIntHandler(SIGINT);
            }
            else if( WIFSIGNALED(status) != 0){
                sigIntHandler(SIGINT);
            }
            else if(WIFSTOPPED(status) != 0){
                sigIntHandler(SIGTSTP);
            }
            tcsetpgrp(0, getpgid(getpid()));

        }
        else if(strcmp(token, "kill") == 0){
            token = strtok(NULL, delim);
            if(*token == '%'){
                token = (token + 1);
                int jid = atoi(token);
                if(jid == 0){
                    printf("sfish builtin error: %s\n", "invalid job");
                    continue;
                }
                int status;
                int pidt = getProcess(jid);
                if(removeProcessPID(pidt) == -1){
                    printf("sfish builtin error: %s\n", "invalid job");
                    continue;
                }
                signal(SIGCHLD, SIG_IGN);
                kill(pidt, SIGKILL);
                waitpid(pidt, &status, 0);
            }
            else{
                int status;
                signal(SIGCHLD, SIG_IGN);
                int tempPID = atoi(token);
                if(removeProcessPID(tempPID) == -1){
                    printf("sfish builtin error: %s\n", "invalid pid");
                    continue;
                }
                kill(tempPID, SIGKILL);
                waitpid(tempPID, &status, 0);
            }
            continue;
        }
        else{
            //EXECUTABLE COMMANDS
            runExec(input, token, dir);
        }

        exited = strcmp(input, "exit") == 0;

        rl_free(input);
    } while(!exited);

    return EXIT_SUCCESS;
}

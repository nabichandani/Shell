#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <readline/readline.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


void exec(char* execInput[], char* input);
int findString(char* str, char* arr[]);
void pipeHelper(char* input, int numPipes, char* dir);
void dividePipe(char* input3, char* input);
char* help = "Help is here!\n";
pid_t pid;
int status;
int count = 1;
char inputHolder[1000];

struct process{
    int JID;
    pid_t pid;
    char jobName[300];
    struct process *next;
};
struct process *allProcesses = NULL;

void addProcess(pid_t newPid, char job[]){
    if(allProcesses == NULL){
        count = 1;
    }
    struct process* newProcess = (struct process*)malloc(sizeof(struct process));
    newProcess->JID = count;
    newProcess->pid = newPid;
    strcpy(newProcess->jobName, job);
    newProcess->next = NULL;
    ++count;
    if(allProcesses == NULL){
        allProcesses = newProcess;
    }
    else{
        struct process *ptr = allProcesses;
        while(ptr->next != NULL){
            ptr = ptr->next;
        }
        ptr->next = newProcess;
    }
}

char* getProcessName(int jid){
    struct process *ptr = allProcesses;
    while(ptr != NULL){
        if(ptr->JID == jid){
            return ptr->jobName;
        }
        ptr = ptr->next;
    }
    return "";
}

pid_t getProcess(int jid){
    struct process *ptr = allProcesses;
    while(ptr != NULL){
        if(ptr->JID == jid){
            return ptr->pid;
        }
        ptr = ptr->next;
    }
    return -1;
}

int removeProcessPID(pid_t delPid){
    if(allProcesses == NULL){
        return -1;
    }
    if(allProcesses->pid == delPid){
        if(allProcesses->next != NULL){
            allProcesses = allProcesses->next;
        }
        else{
            allProcesses = NULL;
        }
        return 0;
    }
    struct process *ptr = allProcesses;
    while(ptr->next != NULL){
        if(ptr->next->pid == delPid){
            if(ptr->next->next == NULL){
                ptr->next = NULL;
            }
            else{
                ptr->next = ptr->next->next;
            }
            return 0;
        }
        ptr = ptr->next;
    }
    return -1;
}

int removeProcessJID(int jid){
    if(allProcesses == NULL){
        return -1;
    }
    if(allProcesses->JID == jid){
        if(allProcesses->next != NULL){
            allProcesses = allProcesses->next;
        }
        else{
            allProcesses = NULL;
        }
        return 0;
    }
    struct process *ptr = allProcesses;
    while(ptr->next != NULL){
        if(ptr->next->JID == jid){
            if(ptr->next->next == NULL){
                ptr->next = NULL;
            }
            else{
                ptr->next = ptr->next->next;
            }
            return 0;
        }
        ptr = ptr->next;
    }
    return -1;
}

void printProcesses(){
    if(allProcesses == NULL){
        return;
    }
    struct process *ptr = allProcesses;
    while(ptr != NULL){
        printf("[%d] %s\n",ptr->JID,ptr->jobName);
        ptr = ptr->next;
    }
}


void sigHandler(int sig){
    if(sig == SIGCHLD){
        waitpid(pid, &status, WUNTRACED | WCONTINUED);
        if(WIFSTOPPED(status) != 0){
            sigHandler(SIGTSTP);
        }
        tcsetpgrp(0, getpgid(getpid()));

    }
    if(sig == SIGINT){
        waitpid(pid, &status, WUNTRACED | WCONTINUED);
        tcsetpgrp(0, getpgid(getpid()));
    }
    if(sig == SIGTSTP){
        tcsetpgrp(0, getpgid(getpid()));
        addProcess(pid, inputHolder);
        printf("[%d] %s\n",count - 1, inputHolder);
    }
}

void runExec(char* input, char* token, char* dir){
    int numPipes = 0;
    memcpy(inputHolder, input, 1000);
    char* execInput[20];
    int inputCount = 0;
    int inputlen = strlen(input);
    char input2[inputlen];
    strcpy(input2, input);
    dividePipe(input2,input);
    char *token2 = strtok(input2, " \t");
    int greater = 0;
    int lesser = 0;
    while(token2 != NULL){
        if(strcmp(token2, "|") == 0){
            numPipes++;
        }
        token2 = strtok(NULL, " \t");
    }



    if(numPipes == 0){
        signal(SIGCHLD, sigHandler);
        signal(SIGTTOU, SIG_IGN);
        signal(SIGTTIN, SIG_IGN);
        signal(SIGTSTP, sigHandler);
        sigset_t currMask, prevMask;
        sigemptyset(&currMask);
        sigaddset(&currMask, SIGCHLD);
        sigprocmask(SIG_BLOCK, &currMask, &prevMask);
        pid = fork();
        if(pid == 0){
            setpgid(getpid(), getpid());
            pid_t pgid = getpgid(getpid());
            tcsetpgrp(0, pgid);
            char input3[inputlen];
            strcpy(input3, input);
            if(strstr(input3, "<") != NULL || strstr(input3, ">") != NULL){
                char tempArr[1000];
                char* space = " ";
                strcpy(tempArr, input);
                char* ptr1 = &input3[0];
                char* ptr2 = &tempArr[0];
                for(int i = 0; i < strlen(input3); i++){
                    if(*ptr2 != '<' && *ptr2 != '>'){
                        memcpy(ptr1, ptr2, 1);
                    }
                    else{
                        if(*ptr2 == '<'){
                            lesser++;
                        }
                        if(*ptr2 == '>'){
                            greater++;
                        }
                        memcpy(ptr1, space, 1);
                        ptr1 = ptr1 + 1;
                        memcpy(ptr1, ptr2, 1);
                        ptr1 = ptr1 + 1;
                        memcpy(ptr1, space, 1);
                    }
                    ptr1 = ptr1 + 1;
                    ptr2 = ptr2 + 1;
                }
                *ptr1 = '\0';
            }
            if(greater > 1 || lesser > 1){
                printf("sfish syntax error: %s\n", "redirection error");
                exit(EXIT_FAILURE);
            }
            token = strtok(input3, " \t");
            while(token != NULL){
                if(strcmp(token, "<") == 0){
                    token = strtok(NULL, " \t");
                    int desc = open(token, O_RDONLY, S_IRWXU);
                    if(desc < 0){
                        dup2(2,0);
                        dup2(2,1);
                        printf("sfish exec error: %s\n", token);
                        exit(EXIT_FAILURE);
                    }
                    dup2(desc, 0);
                    close(desc);
                    token = strtok(NULL, " \t");
                    continue;
                }
                else if(strcmp(token, ">") == 0){
                    token = strtok(NULL, " \t");
                    int desc = open(token, O_WRONLY | O_CREAT | O_TRUNC, S_IRWXU);
                    if(desc < 0){
                        dup2(2,0);
                        dup2(2,1);
                        printf("sfish exec error: %s\n", token);
                        exit(EXIT_FAILURE);
                    }
                    dup2(desc, 1);
                    close(desc);
                    token = strtok(NULL, " \t");
                    continue;
                }
                execInput[inputCount] = token;
                inputCount++;
                token = strtok(NULL, " \t");
            }
            execInput[inputCount] = NULL;
            if(strcmp(execInput[0], "help") == 0){
                write(1, help, strlen(help));
            }
            else if(strcmp(execInput[0], "pwd") == 0){
                char tempDir[1024];
                getcwd(tempDir, 1024);
                strcat(tempDir, "\n");
                write(1, tempDir, strlen(tempDir));
            }
            else{
                exec(execInput, input);
            }
            exit(EXIT_SUCCESS);
        }
        else{
            signal(SIGINT, sigHandler);
            sigsuspend(&prevMask);
            sigprocmask(SIG_SETMASK, &prevMask, NULL);
        }
    }
    else{
        char input4[inputlen];
        strcpy(input4, input);
        dividePipe(input4,input);
        pipeHelper(input4, numPipes, dir);
    }
}


void pipeHelper(char* input, int numPipes, char* dir){
    char* cmd = strtok(input, "|");
    char* cmdArr[1000];
    int i = 0;
    while(cmd != NULL){
        cmdArr[i] = cmd;
        i++;
        cmd = strtok(NULL, "|");
    }
    int pipes[numPipes * 2];
    for(int i = 0; i < numPipes; i++){
        if( pipe(pipes + i*2) < 0 ){
            exit(EXIT_FAILURE);
        }
    }
    int command = 0;
    int cp;
    pid_t cpid;
    signal(SIGCHLD, SIG_IGN);
    while(command < (numPipes + 1)){
        char* cmdInput[20];
        int count = 0;
        char* token = strtok(cmdArr[command], " \t");
        while(token != NULL){
            cmdInput[count] = token;
            count++;
            token = strtok(NULL, " \t");
        }

        cmdInput[count] = NULL;

        cpid = fork();

        if(cpid == 0){
            if(command > 0){
                dup2(pipes[((command - 1) * 2)],0);
            }
            if(command != numPipes){
                dup2(pipes[(command * 2) + 1],1);
            }
            for(int i = 0; i < (2 * numPipes); i++){
                close(pipes[i]);
            }
            if(strcmp(cmdInput[0], "help") == 0){
                write(1, help, strlen(help));
            }
            else if(strcmp(cmdInput[0], "pwd") == 0){
                char tempDir[1024];
                getcwd(tempDir, 1024);
                strcat(tempDir, "\n");
                write(1, tempDir, strlen(tempDir));
            }
            else{
                exec(cmdInput, input);
            }
            exit(EXIT_SUCCESS);
        }
        command++;
    }
    for(int i = 0; i < 2 * numPipes; i++){
        close(pipes[i]);
    }

    waitpid(cpid, &cp, WUNTRACED);
}

void dividePipe(char* input3, char* input){
    strcpy(input3, input);
    char tempArr[1000];
    char* space = " ";
    strcpy(tempArr, input);
    char* ptr1 = &input3[0];
    char* ptr2 = &tempArr[0];
    for(int i = 0; i < strlen(input3); i++){
        if(*ptr2 != '|'){
            memcpy(ptr1, ptr2, 1);
        }
        else{
            memcpy(ptr1, space, 1);
            ptr1 = ptr1 + 1;
            memcpy(ptr1, ptr2, 1);
            ptr1 = ptr1 + 1;
            memcpy(ptr1, space, 1);
        }
        ptr1 = ptr1 + 1;
        ptr2 = ptr2 + 1;
    }
    *ptr1 = '\0';

}

void exec(char* execInput[], char* input){
    int response = execvp(execInput[0], execInput);
    if(response != 0){
        dup2(2,0);
        dup2(2,1);
        printf("sfish: %s: command not found\n", input);
    }
}

void doCD(char* token, char dir[], char prevDir[]){
    token = strtok(NULL, " \t");
    getcwd(dir, 1024);
    if(token == NULL){
        memcpy(prevDir, dir, 1024);
        chdir(getenv("HOME"));
    }
    else if(strcmp(token, "-") == 0){
        chdir(prevDir);
        memcpy(prevDir, dir, 1024);
    }
    else if(strcmp(token, ".") == 0){
        memcpy(prevDir, dir, 1024);
    }
    else if(strcmp(token, "..") == 0){
        memcpy(prevDir, dir, 1024);
        char* removeOne = strrchr(dir, '/');
        if(dir - removeOne + 1 != 1){
            memset(removeOne, 0, dir + 1024 - removeOne);
            chdir(dir);
        }
        else{
            chdir("/");
        }
    }
    else{
        memcpy(prevDir, dir, 1024);
        if(chdir(token) == -1){
            printf("sfish builtin error: %s\n", "invalid dir");
        }
    }
}

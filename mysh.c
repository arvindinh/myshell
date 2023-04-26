//THIS CODE IS MY OWN WORK, IT WAS WRITTEN WITHOUT CONSULTING
//A TUTOR OR CODE WRITTEN BY OTHER STUDENTS - Arvin Dinh
#include "tokens.h"
#include<stdio.h>
#include<stdlib.h>
#include<unistd.h> 
#include<sys/types.h> 
#include<sys/wait.h> 
#include<fcntl.h> 
#include<string.h>
#include<errno.h>

//executes command as normal, but if ampersand, execute in bg.
void execute(char **tokens, int bg) {
    pid_t pid;
    int status;

    pid = fork();

    if (pid < 0) {
        //error
        perror("fork");
        exit(-1);
    } else if (pid == 0) {
        //child
        execvp(tokens[0], tokens);
        perror("exec");
        exit(-1);
    }
    //parent
    if (bg == 0) {
        while (wait(&status) == -1) {
            if (errno == EINTR) {
                continue;
            } else {
                perror("wait");
                exit(-1);
            }
        }

    }
}

//redirects input from file to a command
void inputRedir(char **tokens, int bg) {
    int num_tokens = 0;
    int opp_ind = 0;
    //find where the < occurs to know how to separate the elements
    for( int i=0; tokens[i]; i++ ) {
        if (strcmp(tokens[i], "<") == 0) {
            opp_ind = i;
        }
        num_tokens++;
    }
    char **before = (char**) malloc(sizeof(char*) * (opp_ind));

    int null_ind = 0;
    for (int i = 0; i < opp_ind; i++) {
        before[i] = strdup(tokens[i]);
        null_ind++;
    }
    //null terminate the command after putting it in the smaller array to store it.
    before[null_ind] = NULL;

    int inputfd = open(tokens[opp_ind+1], O_RDONLY);
    if (inputfd == -1) {
        perror("open");
        exit(-1);
    }

    //make a child process to actually execute the command
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(-1);
    }

    if (pid == 0) {
        //redirect input file fd to std in of command
        if (dup2(inputfd, 0) == -1) {
            perror("dup2");
            exit(-1);
        }
        close(inputfd);
        execvp(before[0], before);
        perror("exec");
        exit(-1);
    } else {
        //if no ampersand, execute it foreground
        if (bg == 0) {
            int status;
            while (wait(&status) == -1) {
                if (errno == EINTR) {
                    continue;
                } else {
                    perror("wait");
                    exit(-1);
                }
            }

        }
        free(before);
    }
}

//redirect output from a command as input to a file
void outputRedir(char **tokens, int bg) {
    int num_tokens = 0;
    int opp_ind = 0;
    //same steps as other method, place command into smaller array to pass into execvp
    for( int i=0; tokens[i]; i++ ) {
        if (strcmp(tokens[i], ">") == 0) {
            opp_ind = i;
        }
        num_tokens++;
    }
    //check if file exists
    FILE* fp = fopen(tokens[opp_ind + 1], "r");
    if (fp != NULL) {
        fprintf(stderr, "open(\"%s\"): %s\n", tokens[opp_ind+1], strerror(errno));
        fclose(fp);
        return;
    }
    char **before = (char**) malloc(sizeof(char*) * (opp_ind));

    int null_ind = 0;
    for (int i = 0; i < opp_ind; i++) {
        before[i] = strdup(tokens[i]);
        null_ind++;
    }
    before[null_ind] = NULL;

    //existing or create a file with said permissions
    int outputfd = open(tokens[opp_ind+1], O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
    if (outputfd == -1) {
        perror("open");
        exit(-1);
    }

    //make new child to execute the command, and replace output stream with file descriptor so it writes into the file
    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(-1);
    }

    if (pid == 0) {
        //redirect input file fd to std in of command
        if (dup2(outputfd, 1) == -1) {
            perror("dup2");
            exit(-1);
        }
        close(outputfd);
        execvp(before[0], before);
        perror("exec");
        exit(-1);
    } else {
        
        if (bg == 0) {
            int status;
            while (wait(&status) == -1) {
                if (errno == EINTR) {
                    continue;
                } else {
                    perror("wait");
                    exit(-1);
                }
            }

        }
        free(before);
    }
}

//same as method above outputRedir but append if file exists
int outputAppend(char **tokens, int bg) {
    int num_tokens = 0;
    int opp_ind = 0;
    for( int i=0; tokens[i]; i++ ) {
        if (strcmp(tokens[i], ">>") == 0) {
            opp_ind = i;
        }
        num_tokens++;
    }

    char **before = (char**) malloc(sizeof(char*) * (opp_ind));

    int null_ind = 0;
    for (int i = 0; i < opp_ind; i++) {
        before[i] = strdup(tokens[i]);
        null_ind++;
    }
    before[null_ind] = NULL;

    int outputfd = open(tokens[opp_ind+1], O_WRONLY | O_CREAT | O_APPEND, S_IRUSR | S_IWUSR | S_IROTH | S_IWOTH);
    if (outputfd == -1) {
        perror("open");
        exit(-1);
    }

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        exit(-1);
    }

    if (pid == 0) {
        //redirect input file fd to std in of command
        if (dup2(outputfd, 1) == -1) {
            perror("dup2");
            exit(-1);
        }
        close(outputfd);
        execvp(before[0], before);
        perror("exec");
        exit(-1);
    } else {
        
        if (bg == 0) {
            int status;
            while (wait(&status) == -1) {
                if (errno == EINTR) {
                    continue;
                } else {
                    perror("wait");
                    exit(-1);
                }
            }

        }
        free(before);
    }
}

//transfers output of one command to input of following command separated by pipe operator
void myPipe(char **tokens, int bg) {
    int fd[2];
    int pipeInd = 0;
    
    int i = 0;
    while (tokens[i] != NULL) {
        if (strcmp(tokens[i], "|") == 0) {
            pipeInd = i;
            i++;
            break;
        }
        i++;
    }
    int nextPipeInd = i;
    int beforeSz = 0;
    while (tokens[i] != NULL) {
        if (nextPipeInd == pipeInd+1) {
            beforeSz = pipeInd + 1;
        }
        //calculate next pipe index
        while (tokens[i] != NULL) {
            if (strcmp(tokens[i], "|") == 0) {
                nextPipeInd = i;
                i++;
                break;
            }
            i++;
        }
        if (tokens[i] == NULL) {
            nextPipeInd = i;
        }
        int afterSz = nextPipeInd - pipeInd;

        //create smaller arrays consisting of the elements preceding/succeeding the pipe
        
        char **before = (char**) malloc(sizeof(char*) * beforeSz);
        char **after = (char**) malloc(sizeof(char*) * afterSz);
        
        //fill in mini token arrs
        int ind = 0;

        for (int k = pipeInd - (beforeSz-1); k < pipeInd; k++) {
            before[ind] = strdup(tokens[k]);
            ind++;
        }
        before[beforeSz] = NULL;
        ind = 0;

        for (int j = pipeInd + 1; j < nextPipeInd; j++) {
            after[ind] = strdup(tokens[j]);
            ind++;
        }
        after[afterSz] = NULL;

        if (pipe(fd) == -1) {
            perror("pipe");
            exit(EXIT_FAILURE);
        }
        //first child(before the pipe)
        pid_t pid1 = fork();
        if (pid1 < 0) {
            perror("fork");
            exit(-1);
        }
        if (pid1 == 0) {
            close(fd[0]);
            dup2(fd[1], 1);
            close(fd[1]);
            execvp(before[0], before);
            perror("execvp");
            exit(-1);
        }
        //second child(after the pipe)
        pid_t pid2 = fork();
        if (pid2 < 0) {
            perror("fork");
            exit(-1);
        }
        if (pid2 == 0) {
            close(fd[1]);
            dup2(fd[0], 0);
            close(fd[0]);
            execvp(after[0], after);
            perror("execvp");
            exit(-1);
        }
        close(fd[0]);
        close(fd[1]);
        if (bg == 0) {
            int status;
            wait(&status);  
            wait(&status);  
        }

        free(before);
        free(after);
        pipeInd = nextPipeInd;
        beforeSz = afterSz;
    }


}

int main(int argc, char *argv[]) {
    char *prompt = "mysh";
    if (argc > 1) {
        prompt = argv[1];
        if (argc > 2) {
            fprintf(stderr, "Error: Usage: mysh [prompt]\n");
            exit(-1);
        }
    }
    
    

    while(1) {
        //implement keeping track of indices for commands, maybe just use last flag ind and execute as traversing? pass in current index as parameter?
        int lastFlagInd = 0;
        int bgFlag = 0;
        int inputRedirFlag = 0;
        int outputRedirFlag = 0;
        int appendoutRedirFlag = 0;
        int pipeFlag = 0;
        int oppFlag = 0;
        char str[1024];
        printf("%s: ", prompt);
        fflush(stdout);
        fgets(str, sizeof(str), stdin);

        str[strlen(str)-1] = '\0';
        char *line = (char *) malloc(strlen(str) * sizeof(char));
        strcpy(line, str);
        char **tokens = get_tokens(line);
        free(line);
        for(int i=0; tokens[i]; i++) {
            if (strcmp(tokens[i], "&") == 0) {
                if (bgFlag == 1) {
                    fprintf(stderr, "\"&\" must be last token on command line\n");
                    break;
                }
                bgFlag = 1;
            }
            if (strcmp(tokens[i], "<") == 0) { 
                inputRedirFlag = 1;
                oppFlag ++;
            }
            if (strcmp(tokens[i], ">") == 0) {
                outputRedirFlag = 1;
                oppFlag ++;
            }
            if (strcmp(tokens[i], ">>") == 0) {
                appendoutRedirFlag = 1;
                oppFlag ++;
            }
            if (strcmp(tokens[i], "|") == 0) {
                pipeFlag = 1;
                oppFlag ++;
            }
        }
        
        if (oppFlag == 0) {
            execute(tokens, bgFlag);
        } else if (inputRedirFlag == 1) {
            if (tokens[2] == NULL) {
                if (strcmp(tokens[0], "<") == 0) {
                    fprintf(stderr, "Error: Invalid null command.\n");

                } else {
                    fprintf(stderr, "Error: Missing filename for input redirection. \n");

                }
            } else if (oppFlag > 0) {
                fprintf(stderr, "Error: Ambiguous input redirection. \n");

            } else {
                inputRedir(tokens, bgFlag);
            }
        } else if (outputRedirFlag == 1) {
            if (tokens[2] == NULL) {
                if (strcmp(tokens[0], ">") == 0) {
                    fprintf(stderr, "Error: Invalid null command.\n");

                } else {
                    fprintf(stderr, "Error: Missing filename for output redirection. \n");
                }
                

            } else if (oppFlag > 0) {
                fprintf(stderr, "Error: Ambiguous output redirection. \n");
            } else {
                outputRedir(tokens, bgFlag);
            }
        } else if (appendoutRedirFlag == 1) {
            outputAppend(tokens, bgFlag);
        } else if (pipeFlag == 1) {
            myPipe(tokens, bgFlag);
        }
        
        
        

        free_tokens(tokens);
    }

    return 0;
}

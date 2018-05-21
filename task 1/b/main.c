#include "line_parser.h"
#include "job_control.h"
#include <linux/limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <sys/wait.h>
#include <fcntl.h>

#define COMMAND_MAX_LENGTH 2048


void sig_handler(int sig){
    if (sig == SIGQUIT || sig == SIGCHLD || sig == SIGTSTP)
        printf("\nThe signal %s is ignored. ",strsignal(sig));
    fflush(stdout);
}

int execute(cmd_line *line){
    return execvp(line->arguments[0],line->arguments);
}

int execute_two_command(cmd_line *line){
    int fd[2];
    int cpid1=0,cpid2=0;
    int* status1=0;
    int* status2=0;

    if (pipe(fd) == -1) {
        perror("there was error in open pipe");
        exit(-1);
    }
    cpid1 = fork();
    if (cpid1 == 0) {
        close(1);
        dup(fd[1]);
        close(fd[1]);
        int inputSream, outputStream;
        if (line->input_redirect != NULL) {
            inputSream = open(line->input_redirect, O_RDONLY, 0);
            if (inputSream < 0) {
                perror("there was error in open inputSream");
                exit(-1);
            }
            dup2(inputSream, 0);
        }
        if (line->output_redirect != NULL) {
            outputStream = open(line->output_redirect, O_WRONLY | O_TRUNC | O_CREAT, 0644);
            if (outputStream < 0) {
                perror("there was error in open outputStream");
                exit(-1);
            }
            dup2(outputStream, 1);
        }
        if (execvp(line->arguments[0], line->arguments) == -1) {
            perror("there was error in exev ls");
            exit(-1);
        }
        exit(0);
    }
    close(fd[1]);

    cpid2 = fork();
    if (cpid2 == 0) {
        close(0);
        dup(fd[0]);
        close(fd[0]);
        if ((line->next->next) != 0) {
            execute_two_command(line->next);
        }
        else {
            int inputSream, outputStream;
            if (line->next->input_redirect != NULL) {
                inputSream = open(line->next->input_redirect, O_RDONLY, 0);
                if (inputSream < 0) {
                    perror("there was error in open inputSream");
                    exit(-1);
                }
                dup2(inputSream, 0);
            }
            if (line->next->output_redirect != NULL) {
                outputStream = open(line->next->output_redirect, O_WRONLY | O_TRUNC | O_CREAT, 0644);
                if (outputStream < 0) {
                    perror("there was error in open outputStream");
                    exit(-1);
                }
                dup2(outputStream, 1);
            }
            if (execvp(line->next->arguments[0], line->next->arguments) == -1) {
                perror("there was error in exev tail");
                exit(-1);
            }
        }

            exit(0);
    }
    close(fd[0]);
    waitpid(cpid1, status1, 0);
    waitpid(cpid2, status2, 0);
    return 0;
}
//exevp isnt wotk with ls * becuase is don't know *

job  *getLastItem(job *theList)
{
    if (theList == NULL) // return NULL if list is empty
        return NULL;
    if (theList -> next == NULL) // return 1st element if it's a single element list
        return theList;
    return getLastItem(theList->next); // recurse
}

int main(int argc, char *argv[]) {
    char path[PATH_MAX];
    char com[COMMAND_MAX_LENGTH];
    int cpid=0;
    signal(SIGQUIT, sig_handler);
    signal(SIGCHLD, sig_handler);
    signal(SIGTSTP, sig_handler);
    job* job_list = NULL;

    while(1) {
        getcwd(path, PATH_MAX);
        printf("%s:",path);
        fgets(com, COMMAND_MAX_LENGTH, stdin);
        if(strcmp(com,"quit\n")==0) {
            free_job_list(&job_list);
            exit(0);
        }
        if(strcmp(com,"\n")!=0) {
            if( strcmp(com,"jobs\n")==0){
                update_job_list(&job_list,1);
                print_jobs(&job_list);
            }
            else {
                add_job(&job_list,com);
                cmd_line *command = parse_cmd_lines(com);
                if (command->next == 0) {
                    cpid = fork();
                    if (cpid == 0) {
                        int inputSream, outputStream;
                        if (command->input_redirect != NULL) {
                            inputSream = open(command->input_redirect, O_RDONLY, 0);
                            if (inputSream < 0) {
                                perror("there was error in open inputSream");
                                exit(-1);
                            }
                            dup2(inputSream, 0);
                        }
                        if (command->output_redirect != NULL) {
                            outputStream = open(command->output_redirect, O_WRONLY | O_TRUNC | O_CREAT, 0644);
                            if (outputStream < 0) {
                                perror("there was error in open outputStream");
                                exit(-1);
                            }
                            dup2(outputStream, 1);
                        }
                        if (execute(command) == -1) {
                            perror("there was error in exev");
                            exit(-1);
                        }
                        if (command->output_redirect != NULL)
                            close(inputSream);
                        if (command->input_redirect != NULL)
                            close(outputStream);
                        exit(0);
                    }
                    if (command->blocking == 1) {
                        job* tmp=getLastItem(job_list);
                        waitpid(cpid,&tmp->status , 0);
                    }
                    free_cmd_lines(command);
                } else {
                    execute_two_command(command);
                    free_cmd_lines(command);
                }
            }
        }
        if(job_list != NULL){
            update_job_list(&job_list,1);
        }
    }

    return 0;
}

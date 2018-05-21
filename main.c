#include "line_parser.h"
#include "job_control.h"
#include <linux/limits.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>

#define COMMAND_MAX_LENGTH 2048


void sig_handler(int sig) {
    if (sig == SIGQUIT || sig == SIGCHLD || sig == SIGTSTP)
        //printf("\nThe signal %s is ignored. ",strsignal(sig));
        fflush(stdout);
}

int execute(cmd_line *line) {
    return execv(line->arguments[0], line->arguments);
}

int execute_two_command(cmd_line *line, job *job_list, int first_cpid, job *cur_job) {
    int fd[2];
    int cpid1 = 0, cpid2 = 0;
    int cur_pgid = first_cpid;
    if (pipe(fd) == -1) {
        perror("there was error in open pipe");
        exit(-1);
    }
    cpid1 = fork();
    if (cur_pgid < 0) {
        cur_pgid = cpid1;
        cur_job->pgid = cpid1;
    }
    setpgid(getpid(), cur_pgid);
    if (cpid1 == 0) {
//        signal(SIGTTIN, SIG_DFL);
//        signal(SIGTTOU, SIG_DFL);
//        signal(SIGTSTP, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);
        setpgid(getpid(), cur_pgid);
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
            perror("there was error in exev");
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
            execute_two_command(line->next, job_list, cur_pgid, cur_job);
        } else {
//            signal(SIGTTIN, SIG_DFL);
//            signal(SIGTTOU, SIG_DFL);
//            signal(SIGTSTP, SIG_DFL);
            signal(SIGQUIT, SIG_DFL);
            signal(SIGTSTP, SIG_DFL);
            signal(SIGCHLD, SIG_DFL);
            setpgid(getpid(), cur_pgid);
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
    waitpid(cur_pgid, &cur_job->status, 0);
    return 0;
}
//exevp isnt wotk with ls * becuase is don't know *

job *getLastItem(job *theList) {
    if (theList == NULL) // return NULL if list is empty
        return NULL;
    if (theList->next == NULL) // return 1st element if it's a single element list
        return theList;
    return getLastItem(theList->next); // recurse
}


int execute_multi_command(cmd_line *line, job *job_list, int first_cpid, job *cur_job, int *left_pipe,int *right_pipe,struct termios* shell_tmodes ) {
    int fd[2];
    int cpid = 0;
    int cur_pgid = first_cpid;
    cpid = fork();
    if (cur_pgid < 0) {
        cur_pgid = cpid;
        cur_job->pgid = cpid;
        setpgid(getpid(), cur_pgid);
    }
    if (cpid == 0) {
        int inputSream = 0, outputStream = 0;
        signal(SIGTTIN, SIG_DFL);
        signal(SIGTTOU, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGQUIT, SIG_DFL);
        signal(SIGTSTP, SIG_DFL);
        signal(SIGCHLD, SIG_DFL);
        setpgid(getpid(), cur_pgid);
        if (right_pipe && line->next) {
            close(1);
            dup(right_pipe[1]);
            close(right_pipe[1]);
        }
        if (left_pipe) {
            close(0);
            dup(left_pipe[0]);
            close(left_pipe[0]);
        }
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
            perror("there was error in exev");
            exit(-1);
        }
        waitpid(cur_pgid, &cur_job->status, 0);
        exit(0);
    }
    //close the child working
    if (right_pipe)
        close(right_pipe[1]);
    if (left_pipe)
        close(left_pipe[0]);
    if (line->next) {
        if (pipe(fd) == -1) {
            perror("there was error in open pipe");
            exit(-1);
        }
        execute_multi_command(line->next,job_list,cur_pgid,cur_job,right_pipe,fd,shell_tmodes);
    }
//    if (line->blocking==1)
//        run_job_in_foreground (&job_list,cur_job, 0, shell_tmodes, cur_pgid);
    return 0;
}


int main(int argc, char *argv[]) {
    char path[PATH_MAX];
    char com[COMMAND_MAX_LENGTH];
    int cpid = 0;
    signal(SIGTTIN, SIG_IGN);
    signal(SIGTTOU, SIG_IGN);
    signal(SIGTSTP, SIG_IGN);
    signal(SIGQUIT, sig_handler);
    signal(SIGCHLD, sig_handler);
    signal(SIGTSTP, sig_handler);
    job *job_list = NULL;
    setpgid(getpid(), getpid());
    struct termios *restor_arr = (struct termios *) malloc(sizeof(struct termios));
    tcgetattr(STDIN_FILENO, restor_arr);
    while (1) {
        getcwd(path, PATH_MAX);
        printf("%s:", path);
        if(fgets(com, COMMAND_MAX_LENGTH, stdin)==NULL)
            break;
        if (strcmp(com, "quit\n") == 0) {
            free_job_list(&job_list);
            exit(0);
        }
        if (strcmp(com, "\n") != 0) {
            if (strcmp(com, "jobs\n") == 0) {
                if (job_list == NULL)
                    printf("The job list is empty");
                else
                    print_jobs(&job_list);
            } else {
                cmd_line *command = parse_cmd_lines(com);
                if (strcmp(command->arguments[0],"fg")==0) {
                    if(!(command->arguments[1] || isdigit(command->arguments[1]))){
                        perror("After fg command must come number");
                        exit(-1);
                    }
                    job* working_job = find_job_by_index(job_list, atoi(command->arguments[1]));
                    run_job_in_foreground(&job_list, working_job, 1, restor_arr, getpgid(0));

                } else {
                    if (strcmp(command->arguments[0],"bg")==0) {
                        if(!(command->arguments[1] || isdigit(command->arguments[1]))){
                            perror("After fg command must come number");
                            exit(-1);
                        }
                        run_job_in_background(job_list, atoi(command->arguments[1]));
                    } else {
                        job *cur_job = add_job(&job_list, com);
                        if (command->next == 0) {
                            cpid = fork();
                            cur_job->pgid = cpid;
                            if (cpid == 0) {
                                signal(SIGTTIN, SIG_DFL);
                                signal(SIGTTOU, SIG_DFL);
                                signal(SIGTSTP, SIG_DFL);
                                signal(SIGQUIT, SIG_DFL);
                                signal(SIGTSTP, SIG_DFL);
                                signal(SIGCHLD, SIG_DFL);
                                setpgid(getpid(), getpid());
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
                                if (execvp(command->arguments[0], command->arguments) == -1) {
                                    perror("there was error in exev");
                                    exit(-1);
                                }
                                exit(0);
                            }
                            setpgid(getpid(), cpid);
                            if (command->blocking == 1) {
                                // waitpid(cur_job->pgid, &cur_job->status, WUNTRACED);
//                            run_job_in_foreground (&job_list,cur_job, 0, restor_arr, cpid);
                            }
                            free_cmd_lines(command);
                        } else {
                            int fd[2];
                            if (pipe(fd) == -1) {
                                perror("there was error in open pipe");
                                exit(-1);
                            }
                            execute_multi_command(command, job_list, -1, cur_job, NULL, fd, restor_arr);
                            if (command->blocking == 1) {
                                run_job_in_foreground(&job_list, cur_job, 0, restor_arr, cpid);
                            }

                            free_cmd_lines(command);
                        }
                    }
                }
            }
        }
    }

    return 0;
}
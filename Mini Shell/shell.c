#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>


int is_pipe(char **arglist);
int single_piping(int index, char **arglist);
int input_redirecting(int count, char **arglist);
int output_redirecting(int count, char **arglist);


int prepare(void){
    if(signal(SIGINT, SIG_IGN) == SIG_ERR){ //Parent will ignore SIGINT
        fprintf(stderr, "Error: signal(SIGINT) has FAILED in prepare()\n");
        return -1;
    }
    if(signal(SIGCHLD, SIG_IGN) == SIG_ERR){ //no zombies, taken from Notes in man7.org/linux/man-pages/man2/wait.2.html
        fprintf(stderr, "Error: signal(SIGCHLD) has FAILED in prepare()\n");
        return -1;
    }
    return 0;
}

int process_arglist(int count, char **arglist){
    pid_t pid;
    int index;
    /***Background command***/
    if (strcmp(arglist[count-1],"&") == 0){
        pid = fork();
        if (pid < 0){ //fork failed
            fprintf(stderr, "Error: fork() has FAILED.\n");
            return 0;
        }
        if (pid == 0){ //child process
            arglist[count-1] = NULL;
            if(signal(SIGCHLD, SIG_DFL) == SIG_ERR){ //back to defualt
                fprintf(stderr, "Error: signal(SIGCHLD) has FAILED in child\n");
                exit(1);
            }
            execvp(arglist[0],arglist);
            //Error! shouldn't get here.
            fprintf(stderr, "Error: execvp() has FAILED.\n");
            exit(1);
        }
        //parent process
        return 1;
    }
    /***Piping***/
    index = is_pipe(arglist);
    if (index != -1){
        return single_piping(index, arglist);
    }
    /***Input redirection***/
    if (count > 1 && strcmp(arglist[count-2], "<") == 0){
        return input_redirecting(count, arglist);
    }
    /***Output redirection***/
    if (count > 1 && strcmp(arglist[count-2], ">") == 0){
        return output_redirecting(count, arglist);
    }
    /***Execute command***/
    pid = fork();
    if (pid < 0){ //fork failed
        fprintf(stderr, "Error: fork() has FAILED.\n");
        return 0;
    }
    if (pid == 0) { // child process
        if (signal(SIGINT, SIG_DFL) == SIG_ERR){ //Foreground child should terminate upon SIGINT
            fprintf(stderr, "Error: signal() has FAILED in child\n");
            exit(1);
        }
        if (signal(SIGCHLD, SIG_DFL) == SIG_ERR){
            fprintf(stderr, "Error: signal(SIGCHLD) has FAILED in child\n");
            exit(1);
        }
        execvp(arglist[0], arglist);
        fprintf(stderr, "Error: execvp() has FAILED.\n");
        exit(1);
    }
    //parent process
    if(waitpid(pid, NULL, 0) == -1 && errno != EINTR && errno != ECHILD){
        fprintf(stderr, "Error: waitpid has FAILED.\n");
        return 0;
    }
    return 1;
}



int finalize(void){
    return 0;
}

int is_pipe(char **arglist){
    int i = 0;
    while (arglist[i] != NULL){
        if (strcmp(arglist[i], "|") == 0){
            return i;
        }
        i++;
    }
    return -1;
}


int single_piping(int index, char **arglist){ //Assisted by ChatGPT
    arglist[index] = NULL;
    pid_t child1_pid, child2_pid;
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        fprintf(stderr, "Error: pipe has FAILED.\n");
        return 0;
    }

    child1_pid = fork();

    if (child1_pid < 0){ //fork failed
        fprintf(stderr, "Error: fork child1 has FAILED.\n");
        return 0;
    }
    if (child1_pid == 0){ //child1 process
        if (signal(SIGINT, SIG_DFL) == SIG_ERR){ //"Foreground child should terminate upon SIGINT"
            fprintf(stderr, "Error: signal() has FAILED in child1\n");
            exit(1);
        }
        if(signal(SIGCHLD, SIG_DFL) == SIG_ERR){
            fprintf(stderr, "Error: signal(SIGCHLD) has FAILED in child1\n");
            exit(1);
        }
        close(pipefd[0]);  // Close read end of the pipe
        if (dup2(pipefd[1], STDOUT_FILENO) == -1){// Redirect stdout to the write end of the pipe
            fprintf(stderr, "Error: dup2() has FAILED in child1\n");
            exit(1);
        }
        close(pipefd[1]);  // Close the original write end of the pipe
        execvp(arglist[0],arglist);
        //Error! shouldn't get here.
        fprintf(stderr, "Error: execvp() in child1 has FAILED.\n");
        exit(1);
    }
    //parent process
    child2_pid = fork();

    if (child2_pid < 0){ //fork failed
        fprintf(stderr, "Error: fork child2 has FAILED.\n");
        return 0;
    }
    if (child2_pid == 0){ //child2 process
        if (signal(SIGINT, SIG_DFL) == SIG_ERR){ //"Foreground child should terminate upon SIGINT"
            fprintf(stderr, "Error: signal() has FAILED in child2\n");
            exit(1);
        }
        if(signal(SIGCHLD, SIG_DFL) == SIG_ERR){ 
            fprintf(stderr, "Error: signal(SIGCHLD) has FAILED in child2\n");
            exit(1);
        }
        close(pipefd[1]);  // Close write end of the pipe
        if (dup2(pipefd[0], STDIN_FILENO) == -1){// Redirect stdin to the read end of the pipe
            fprintf(stderr, "Error: dup2() has FAILED in child2\n");
            exit(1);
        }
        close(pipefd[0]);  // Close the original read end of the pipe
        execvp(arglist[index+1],&arglist[index+1]);
        //Error! shouldn't get here.
        fprintf(stderr, "Error: execvp() in child2 has FAILED.\n");
        exit(1);
    }
    //back to Parent after fork the 2nd child
    // Close both ends of the pipe in the parent process
    close(pipefd[0]);
    close(pipefd[1]);
    // Wait for both child processes to finish
    if(waitpid(child1_pid, NULL, 0) == -1 && errno != EINTR && errno != ECHILD){
        fprintf(stderr, "Error: waitpid for child1 has FAILED.\n");
        return 0;
    }
    if(waitpid(child2_pid, NULL, 0) == -1 && errno != EINTR && errno != ECHILD){
        fprintf(stderr, "Error: waitpid for child2 has FAILED.\n");
        return 0;
    }
    return 1;
}

int input_redirecting(int count, char **arglist){
    arglist[count-2] = NULL;
    pid_t pid;
    pid = fork();
    if (pid < 0){ //fork failed
        fprintf(stderr, "Error: fork has FAILED.\n");
        return 0;
    }
    if (pid == 0){ //child process
        if (signal(SIGINT, SIG_DFL) == SIG_ERR){ //Foreground child should terminate upon SIGINT
            fprintf(stderr, "Error: signal() has FAILED\n");
            exit(1);
        }
        if(signal(SIGCHLD, SIG_DFL) == SIG_ERR){ 
            fprintf(stderr, "Error: signal(SIGCHLD) has FAILED in child\n");
            exit(1);
        }
        int fd = open(arglist[count-1], O_RDONLY); //file descriptor
        if (fd == -1){
            fprintf(stderr, "Error: open has FAILED in child.\n");
            exit(1);
        }
        if (dup2(fd, STDIN_FILENO) == -1){// Redirect stdin
            fprintf(stderr, "Error: dup2 has FAILED\n");
            exit(1);
        }
        close(fd);  // Close the original file descriptor after redirection
        execvp(arglist[0],arglist);
        //Error! shouldn't get here.
        fprintf(stderr, "Error: execvp() has FAILED.\n");
        exit(1);
    }
    //parent process
    if(waitpid(pid, NULL, 0) == -1 && errno != EINTR && errno != ECHILD){
        fprintf(stderr, "Error: waitpid has FAILED.\n");
        return 0;
    }
    return 1;
}

int output_redirecting(int count, char **arglist){
    arglist[count-2] = NULL;
    pid_t pid;
    pid = fork();
    if (pid < 0){ //fork failed
        fprintf(stderr, "Error: fork has FAILED.\n");
        return 0;
    }
    if (pid == 0){ //child process
        if (signal(SIGINT, SIG_DFL) == SIG_ERR){ //Foreground child should terminate upon SIGINT
            fprintf(stderr, "Error: signal(SIGINT) has FAILED\n");
            exit(1);
        }
        if(signal(SIGCHLD, SIG_DFL) == SIG_ERR){
            fprintf(stderr, "Error: signal(SIGCHLD) has FAILED in child\n");
            exit(1);
        }
        int fd = open(arglist[count-1], O_WRONLY | O_CREAT | O_TRUNC, 0644); //Assisted by ChatGPT
        if (fd == -1){
            fprintf(stderr, "Error: open has FAILED in child.\n");
            exit(1);
        }
        if (dup2(fd, STDOUT_FILENO) == -1){// Redirect stdout
            fprintf(stderr, "Error: dup2 has FAILED\n");
            exit(1);
        }
        close(fd);
        execvp(arglist[0],arglist);
        //Error! shouldn't get here.
        fprintf(stderr, "Error: execvp() has FAILED.\n");
        exit(1);
    }
    //parent process
    if(waitpid(pid, NULL, 0) == -1 && errno != EINTR && errno != ECHILD){
        fprintf(stderr, "Error: waitpid has FAILED.\n");
        return 0;
    }
    return 1;
}
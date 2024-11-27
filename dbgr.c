#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <readline/readline.h>
#include <readline/history.h>


typedef struct {
    pid_t target_pid; //PID of process being debugged
    int is_running;   //Flag to indicate if process is running
    char target_executable[256]; //path to executable
    uintptr_t breakpoint_addr; //address of a single breakpoint
    int breakpoint_inst; //original instruction at the breakpoint address
    int status; //last status of target process
} debugger; 

void handle_command(char *line) {
    char *args;
    char *args = strtok_t(line, " ", &args);
    char *command = args[0];
    if(is_prefix(command, "continue")) {
        continue_execution();
    } else {
        fprintf(stderr, "unknown command!\n");
    }
}



void run(debugger vm) {
    int wait_status;
    void *options;
    waitpid(target_pid, &wait_status, options);
    char *line;
    while((line = readline("patchdbg> ")) != NULL) {
        add_history(line);
    }
}


int main(int argc, char *argv[]) {
    if(argc < 2) {
        fprintf(stderr, "PROGRAM NOT SPECIFIED!\n");
        return -1;
    }
    
    char *prog = argv[1];

    pid_t pid = fork();
    if(pid == 0) {
        //we are in child process
        //execute debugee
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execl(prog, prog, NULL);
    } else if (pid >= 1) {
        //we are in parent process
        //execute debugger
        fprintf(stderr, "Started debugging process %d", pid);
    }

}

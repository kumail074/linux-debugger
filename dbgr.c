#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>

struct debugger {
    pid_t targetPid;
    int is_running;
    char target_executable[256];
    uintptr_t breakpoint_addr;
    int breakpoint_inst;
    int status;
};


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
    } else if (pid >= 1) {
        //we are in parent process
        //execute debugger
        fprintf(stderr, "Started debugging process %d", pid);
    }

}

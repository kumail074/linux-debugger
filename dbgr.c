#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>


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
    }

}

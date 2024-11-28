#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/types.h>

/*typedef struct {
    pid_t target_pid; //PID of process being debugged
    int is_running;   //Flag to indicate if process is running
    char target_executable[256]; //path to executable
    uintptr_t breakpoint_addr; //address of a single breakpoint
    int breakpoint_inst; //original instruction at the breakpoint address
    int status; //last status of target process
} debugger;*/

typedef struct {
    pid_t prog_pid;
    char *prog_name;
} debugger;

void continue_execution(debugger *dbg) {
    ptrace(PTRACE_CONT, dbg->prog_pid, NULL, NULL);

    int wait_status;
    int options = 0;
    waitpid(dbg->prog_pid, &wait_status, options);
}

bool is_prefix(const char *s, const char *of) {
    size_t s_len = strlen(s);
    size_t of_len = strlen(of);
    if(s_len > of_len) return false;

    return strncmp(s, of, s_len) == 0;
}

void handle_command(debugger *dbg, char *line) {
    char delim[] = " ";
    char *args = strtok(line, delim);
    if(args != NULL) {
        char command[50];
        strncpy(command, args, sizeof(command) - 1);
        command[sizeof(command) - 1] = '\0';
    }

    if(is_prefix(command, "continue")) {
        continue_execution(dbg);
    } else {
        fprintf(stderr, "unknown command\n");
    }
}

void run(debugger *dbg) {
    int wait_status;
    int options = 0;
    waitpid(dbg->prog_pid, &wait_status, options);
    char *line = NULL;
    while((line = readline("ldb> ")) != NULL) {
        handle_command(dbg, line);
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
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execl(prog, prog, 0);
    } else if(pid >= 1) {
        fprintf(stderr, "Started debugging process (%d)...\n", pid);
        debugger dbg = { .prog_pid = pid,
            .prog_name = prog };
        run(&dbg);
    }
}

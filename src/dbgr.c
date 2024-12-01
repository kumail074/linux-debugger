#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stddef.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <stdint.h>
#include <inttypes.h>
#include <readline/readline.h>
#include <readline/history.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/user.h>
#include "hash/hashb.h"

/*typedef struct {
    pid_t target_pid; //PID of process being debugged
    int is_running;   //Flag to indicate if process is running
    char target_executable[256]; //path to executable
    uintptr_t breakpoint_addr; //address of a single breakpoint
    int breakpoint_inst; //original instruction at the breakpoint address
    int status; //last status of target process
} debugger;*/

enum reg {
    rax, rbx, rcx, rdx,
    rdi, rsi, rbp, rsp,
    r8, r9, r10, r11,
    r12, r13, r14, r15,
    rip, rflags, cs,
    orig_rax, fs_base,
    gs_base,
    fs, gs, ss, ds, es
};

#define N_REGISTERS 27

typedef struct {
    enum reg r;
    int dwarf_r;
    char *name;
} reg_descriptor;

reg_descriptor g_register_description[N_REGISTERS] = {
    { r15, 15, "r15" },
    { r14, 14, "r14" },
    { r13, 13, "r13" },
    { r12, 12, "r12" },
    { rbp,  6, "rbp" },
    { rbx,  3, "rbx" },
    { r11, 11, "r11" },
    { r10, 10, "r10" },
    {  r9,  9,  "r9" },
    {  r8,  8,  "r8" },
    { rax,  0, "rax" },
    { rcx,  2, "rcx" },
    { rdx,  1, "rdx" },
    { rsi,  4, "rsi" },
    { rdi,  5, "rdi" },
    { orig_rax, -1, "orig_rax" },
    { rip, -1, "rip" },
    {  cs, 51,  "cs" },
    { rflags, 49, "rflags" },
    { rsp,  7, "rsp" },
    {  ss, 52,  "ss" },
    { fs_base, 58, "fs_base" },
    { gs_base, 59, "gs_base" },
    {  ds, 53,  "ds" },
    {  es, 50,  "es" },
    {  fs, 54,  "fs" },
    {  gs, 55,  "gs" }
};

typedef struct {
    pid_t prog_pid;
    char *prog_name;
    hash_table m_breakpoint;
} debugger;

typedef struct {
    pid_t prog_pid;
    intptr_t m_addr;
    bool m_enabled;
    uint8_t saved_data;
} breakpoint;
   
uint64_t get_register_value(pid_t pid, enum reg r) {
    struct user_regs_struct regs;

    if(ptrace(PTRACE_GETREGS, pid, NULL, &regs) == -1) {
        perror("ptrace(PTRACE_GETREGS)");
        return 0;
    }
    for(size_t i = 0; i < N_REGISTERS; i++) {
        if(g_register_description[i].r == r) {
            uint64_t *regptr = (uint64_t *)((char *)&regs + g_register_description[i].dwarf_r);
            return *regptr;
        }
    }
    fprintf(stderr, "Register not found\n");
    return 0;
}

void set_register_value(pid_t pid, enum reg r, uint64_t value) {
    struct user_regs_struct regs;
    reg_descriptor *target = NULL;
    ptrace(PTRACE_GETREGS, pid, NULL, &regs);
    for(size_t i = 0; i < N_REGISTERS; i++) {
        if(g_register_description[i].r == r) {
            target = &g_register_description[i];
            break;
        }
    }
    if(target == NULL) {
        fprintf(stderr, "error: register not found.\n");
        return;

    }
    uint64_t register_ptr = (uint64_t)((char*)&regs + offsetof(struct user_regs_struct, rax) + (target - g_register_description));
    register_ptr = value;
    if(ptrace(PTRACE_SETREGS, pid, NULL, &regs) == -1) {
        perror("ptrace(SETREGS)");
        return;
    }
}

uint64_t get_register_value_from_dwarf_register(pid_t pid, unsigned regum) {
    for(size_t i = 0; i < N_REGISTERS; i++) {
        if(g_register_description[i].dwarf_r == regum) {
            return get_register_value(pid, g_register_description[i].r);
        }
    }
    fprintf(stderr, "error: unknown dwarf register.\n");
    exit(EXIT_FAILURE);
        
}

char* get_register_name(enum reg r) {
    for(size_t i = 0; i < N_REGISTERS; i++) {
        if(g_register_description[i].r == r) {
            return g_register_description[i].name;
            break;
        }
    }
    fprintf(stderr, "error: not found.\n");
    exit(EXIT_FAILURE);
}

enum reg get_register_from_name(const char* name) {
    for(size_t i = 0; i < N_REGISTERS; i++) {
        if(g_register_description[i].name == name){
            return g_register_description[i].r;
            break;
        }
    }
    fprintf(stderr, "error: not found.\n");
    exit(EXIT_FAILURE);
}

void dump_registers(pid_t pid) { //debugger function
    for(int i = 0; g_register_description[i].name != NULL; i++) {
        reg_descriptor *rd = &g_register_description[i];
        uint64_t reg_value = get_register_value(pid, rd->r);
        printf("%s 0x%016" PRIx64 "\n", rd->name, rd->dwarf_r);
    }
}

bool is_enabled(breakpoint *dbg) {  //breakpoint function
    return dbg->m_enabled;
}

intptr_t get_address(breakpoint *dbg) { ///breakpoint funtion
    return dbg->m_addr;
}

void breakpoint_disable(breakpoint *dbg) { //breakpoint function
    long data = ptrace(PTRACE_PEEKDATA, dbg->prog_pid, dbg->m_addr, NULL);
    uint64_t restored_data = ((data & ~0xff) | dbg->saved_data);
    ptrace(PTRACE_POKEDATA, dbg->prog_pid, dbg->m_addr, restored_data);
    dbg->m_enabled = false;
}

void breakpoint_enable(breakpoint *dbg) {  //breakpoint function
    long data = ptrace(PTRACE_PEEKDATA, dbg->prog_pid, dbg->m_addr, NULL);
    dbg->saved_data = (uint8_t) data & 0xff;

    uint64_t int3 = 0xcc;
    uint64_t data_int3 = ((data & ~0xff) | int3);
    ptrace(PTRACE_POKEDATA, dbg->prog_pid, dbg->m_addr, data_int3);
   dbg->m_enabled = true;
} 

void set_breakpoint_at_address(debugger *dbg, intptr_t addr) {
    fprintf(stderr, "set breakpoint at address 0x%" PRIxPTR "\n", (intptr_t) addr);
    breakpoint bp;
    breakpoint_enable(&bp);
    add_breakpoint(&dbg->m_breakpoint, addr, NULL);
}

void continue_execution(debugger *dbg) {  //debugger function
    ptrace(PTRACE_CONT, dbg->prog_pid, NULL, NULL);

    int wait_status;
    int options = 0;
    waitpid(dbg->prog_pid, &wait_status, options);
}

bool is_prefix(char *s, const char *of) { //debugger function
    size_t s_len = strlen(s);
    size_t of_len = strlen(of);
    if(s_len > of_len) return false;

    return strncmp(s, of, s_len) == 0;
}

void handle_command(debugger *dbg, char *line) { //debugger function
    char delim[] = " ";
    char *args = strtok(line, delim);
    if(args != NULL) {
        char command[50];
        strncpy(command, args, sizeof(command) - 1);
        command[sizeof(command) - 1] = '\0';
    }

    if(is_prefix(&args[0], "cont")) {
        continue_execution(dbg);
    } else if (is_prefix(&args[0], "break")) {
        char addr[3];
        strncpy(addr, &args[1], 2);
        intptr_t address = strtol(addr, NULL, 16);
        set_breakpoint_at_address(dbg, address);
    } else {
        fprintf(stderr, "unknown command\n");
    }
}

void run(debugger *dbg) { //init function
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

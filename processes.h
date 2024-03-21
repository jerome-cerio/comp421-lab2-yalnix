#ifndef PROCESSES_H
#define PROCESSES_H

#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdint.h>

// The states a process may be in.
enum state_t {READY, RUNNING, DELAYED, WAITING, READING, WRITING, TERMINATED};

// Structure of a PCB
struct pcb {
    unsigned int pid;
    enum state_t state;
    uintptr_t ptaddr0;
    int used_npg;
    uintptr_t user_stack_base;
    uintptr_t brk;
    SavedContext ctx;
};

struct pcb_node {
    struct pcb *process;
    struct pcb_node *next;
};

// Storing the active process.
struct pcb *active;

// Details of the "idle" and "init" processes
struct pcb *idle;
struct pcb *init;

#endif
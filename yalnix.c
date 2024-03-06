#include <stdio.h>
#include <stdlib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

// Type definition for an interrupt handler
typedef void (*interrupt_handler_t)(ExceptionInfo);

struct interrupt_vector_table {
    interrupt_handler_t interrupt_handlers[TRAP_VECTOR_SIZE - 1];
}

struct pcb {

}

struct terminal{

}

void KernelStart(ExceptionInfo *info, unsigned int pmem_size, void *orig_brk, char **cmd_args);
int LoadProgram(char *name, char **args, ExceptionInfo *info);
int SetKernelBrk(void *addr);

/* Interrupt handlers*/
void trap_kernel_handler(ExceptionInfo *info);
void trap_clock_handler(ExceptionInfo *info);
void trap_illegal_handler(ExceptionInfo *info);
void trap_memory_handler(ExceptionInfo *info);
void trap_math_handler(ExceptionInfo *info);
void trap_tty_receive_handler(ExceptionInfo *info);
void trap_tty_transmit_handler(ExceptionInfo *info);


SavedContext *MySwitchFunc(SavedContext *ctxp, void *p1, void *p2);
SavedContext *MyCFunc(SavedContext *ctxp, void *p1, void *p2);


void KernelStart(ExceptionInfo *info, unsigned int pmem_size, void *orig_brk, char **cmd_args) {

    // Initialize structures

    // Construct page tables

    // Create the idle process - pid0

    // Create an init process - pid1

    // Enable virtual memory
    WriteRegister(REG_VM_ENABLE, 1)
}

void trap_kernel_handler(ExceptionInfo *info) {

}

void trap_clock_handler(ExceptionInfo *info) {

}

void trap_illegal_handler(ExceptionInfo *info) {

}

void trap_memory_handler(ExceptionInfo *info) {

}

void trap_math_handler(ExceptionInfo *info) {

}

void trap_tty_receive_handler(ExceptionInfo *info) {

}

void trap_tty_transmit_handler(ExceptionInfo *info) {

}

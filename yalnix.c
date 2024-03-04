#include <stdio.h>
#include <stdlib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>

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

    // Initialize interrupt vector table
    void (*interrupt_vector_table[TRAP_VECTOR_SIZE])(ExceptionInfo *info); 

    interrupt_vector_table[0] = trap_kernel_handler; 
    interrupt_vector_table[1] = trap_clock_handler; 
    interrupt_vector_table[2] = trap_illegal_handler; 
    interrupt_vector_table[3] = trap_memory_handler; 
    interrupt_vector_table[4] = trap_math_handler; 
    interrupt_vector_table[5] =  trap_tty_receive_handler; 
    interrupt_vector_table[6] = trap_tty_transmit_handler; 
    interrupt_vector_table[7] = null; 
    interrupt_vector_table[8] = null; 
    interrupt_vector_table[9] = null; 
    interrupt_vector_table[10] = null; 
    interrupt_vector_table[11] = null; 
    interrupt_vector_table[12] = null; 
    interrupt_vector_table[13] = null; 
    interrupt_vector_table[14] = null; 
    interrupt_vector_table[15] = null; 

    // Initialize the REG_VECTOR_BASE to point to interrupt vector table 
    WriteRegister(REG_VECTOR_BASE, (RCS421RegVal) interrupt_vector_table); 

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

#include <stdio.h>
#include <stdlib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>



// Type definition for an interrupt handler
typedef void (*interrupt_handler_t)(ExceptionInfo);

// Struct for the interrupt vector table holding trap handlers 
struct interrupt_vector_table {
    interrupt_handler_t interrupt_handlers[TRAP_VECTOR_SIZE - 1];
};

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

    TracePrintf(1, "Entering KernelStart");

    // Create the interrupt vector table
    void (*interrupt_vector_table[TRAP_VECTOR_SIZE])(ExceptionInfo *info); 

    // Initialize the interrupt vector table entries
    for (int i = 0; i < TRAP_VECTOR_SIZE; i++) {
        switch (i) {
            case TRAP_KERNEL:
                interrupt_vector_table[i] = trap_kernel_handler;
                break;
            case TRAP_CLOCK:
                interrupt_vector_table[i] = trap_clock_handler;
                break;
            case TRAP_ILLEGAL:
                interrupt_vector_table[i] = trap_illegal_handler;
                break;
            case TRAP_MEMORY:
                interrupt_vector_table[i] = trap_memory_handler;
                break;
            case TRAP_MATH:
                interrupt_vector_table[i] = trap_math_handler;
                break;
            case TRAP_TTY_RECEIVE:
                interrupt_vector_table[i] = trap_tty_receive_handler;
                break;
            case TRAP_TTY_TRANSMIT:
                interrupt_vector_table[i] = trap_tty_transmit_handler;
                break;
            default:
                interrupt_vector_table[i] = NULL;
                break;
        }

    }

    // Initialize the REG_VECTOR_BASE to point to interrupt vector table 
    WriteRegister(REG_VECTOR_BASE, (RCS421RegVal) interrupt_vector_table); 

    // Create the list of free physical pages
    initFreePagesList(pmem_size);

    // Initialize the Region 0 and Region 1 page tables
    initPageTables();

    // Create the idle process - pid0

    // Create an init process - pid1

    // Enable virtual memory
    WriteRegister(REG_VM_ENABLE, 1);
    vm_enabled = 1; 
}

// Procedure called by malloc to add more pages to kernel's memory 
int SetKernelBrk(void *addr) {

    // Virtual memory not yet enabled, give more pages in physical memory 
    if (vm_enabled == 0) {
        current_break = addr; 
    
    // Virtual memory has been enabled, give more pages in virtual memory 
    } else {

    }
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

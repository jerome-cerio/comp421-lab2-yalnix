#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <comp421/yalnix.h>
#include <comp421/hardware.h>
#include <comp421/loadinfo.h>

#define VALID   1 
#define INVALID 0
#define FREE 1
#define UNFREE 0         

// Type definition for an interrupt handler
typedef void (*interrupt_handler_t)(ExceptionInfo);

// Struct for the interrupt vector table holding trap handlers 
struct interrupt_vector_table {
    interrupt_handler_t interrupt_handlers[TRAP_VECTOR_SIZE - 1];
};

// Global variables 
int vm_enabled = 0; 
int num_free_pages = 0;
int num_pages;  

// Points to the current kernel break which starts at orig_brk 
void *current_break;  

// Array to keep track of free pages 
int *free_page_tracker; 

// Declaration of page tables 
struct pte *region0_pt; 
struct pte *region1_pt; 

struct pcb {
};

struct terminal{
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

/*
 *  This function initializes the array used to track the status
 *  of pages (whether they are free or not). 
 */
void initFreePagesList(unsigned int pmem_size) {
    int i; 

    // Find the already used pages
    int num_physical_pages = pmem_size >> PAGESHIFT;

    free_page_tracker = malloc(num_physical_pages * sizeof(int)); 

    int used_pages_min = DOWN_TO_PAGE(VMEM_1_BASE) / PAGESIZE;
    int used_pages_max = UP_TO_PAGE(current_break) / PAGESIZE;

    for (i = MEM_INVALID_PAGES; i < num_physical_pages; i++) {
        
        if (i < used_pages_min || i > used_pages_max) {
            // The page is free. Add it to the free list

            free_page_tracker[i] = FREE; 
            num_free_pages++;
        }
    }
}

/*
 *  This function "gets" the next free page. 
 *
 *  Returns:
 *      Index of the free page that has been gotten on success 
 *     -1 if there aren free pages remaining at the moment 
 */
int getFreePage() {
    int i; ; 
    int free_page = -1; 

    for (i = 0; i < num_pages; i++) {

        if (free_page_tracker[i] == FREE) {

            free_page = i; 
            free_page_tracker[i] = UNFREE;
            num_free_pages--;  
            break; 
        }
    }

    return free_page; 
}

void createRegion0PT(void *addr) {


}

void KernelStart(ExceptionInfo *info, unsigned int pmem_size, void *orig_brk, char **cmd_args) {
    int i; 
    current_break = orig_brk; 
    num_pages = pmem_size >> PAGESHIFT; 

    TracePrintf(1, "Entering KernelStart");

    // Create the interrupt vector table
    void (*interrupt_vector_table[TRAP_VECTOR_SIZE])(ExceptionInfo *info); 

    // Initialize the interrupt vector table entries
    for (i = 0; i < TRAP_VECTOR_SIZE; i++) {
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
    region0_pt = (struct pte *) VMEM_1_LIMIT - PAGESIZE; 
    region1_pt = (struct pte *) VMEM_1_LIMIT - (PAGESIZE * 2);

    for (i = 0; i < VMEM_LIMIT >> PAGESHIFT; i++) {

        // Before Region 0 kernel stack in memory 
        if (i < KERNEL_STACK_BASE >> PAGESHIFT) {
            region0_pt[i].pfn = -1; 
            region0_pt[i].kprot = PROT_NONE; 
            region0_pt[i].uprot = PROT_NONE; 
            region0_pt[i].valid = INVALID; 

        // In Region 0 kernel stack in memory 
        } else if (i < KERNEL_STACK_LIMIT >> PAGESHIFT) {
            region0_pt[i].pfn = i; 
            free_page_tracker[i] = UNFREE; 
            region0_pt[i].kprot = PROT_READ | PROT_WRITE; 
            region0_pt[i].uprot = PROT_NONE; 
            region0_pt[i].valid = VALID; 

        // In Region 1 kernel text pages of kernel heap in memory 
        } else if (i < (long int) &_etext >> PAGESHIFT) {
            region1_pt[i].pfn = i; 
            free_page_tracker[i] = UNFREE; 
            region1_pt[i].kprot = PROT_EXEC; 
            region1_pt[i].uprot = PROT_NONE; 
            region1_pt[i].valid = VALID; 

        // In Region 1 kernel data/bss/heap pages of kernel heap in memory 
        } else if (i < (long int) current_break >> PAGESHIFT) {
            region1_pt[i].pfn = i; 
            free_page_tracker[i] = UNFREE; 
            region1_pt[i].kprot = PROT_READ | PROT_WRITE; 
            region1_pt[i].uprot = PROT_NONE; 
            region1_pt[i].valid = VALID; 

        // In Region 1 space under our allocated data structures in memory 
        } else if (i < VMEM_1_LIMIT - (PAGESIZE * 2)) {
            region1_pt[i].pfn = -1; 
            region1_pt[i].kprot = PROT_NONE; 
            region1_pt[i].uprot = PROT_NONE; 
            region1_pt[i].valid = INVALID; 

        // In Region 1 space with our allocated data stuctures in memory 
        } else {
            region1_pt[i].pfn = i; 
            free_page_tracker[i] = UNFREE; 
            region1_pt[i].kprot = PROT_READ | PROT_WRITE; 
            region1_pt[i].uprot = PROT_NONE; 
            region1_pt[i].valid = VALID; 
        }
    }

    // Initialize the page table registers to point to their respective page tables 
    WriteRegister(REG_PTR0, (RCS421RegVal) region0_pt); 
    WriteRegister(REG_PTR1, (RCS421RegVal) region1_pt); 

    // Enable virtual memory
    WriteRegister(REG_VM_ENABLE, 1); 
    vm_enabled = 1; 

    // Create the idle process - pid0
    LoadProgram("idle", cmd_args, info); 

    // NOT DONEEEE CONTEXT SWITCHING NEEDS TO HAPPEN
    // THERE ARE DIFF TYPES OF CONTEXT SWITCH FUNCTIONS 
    // NEW REGION 0 PAGE TABLE CREATION OR COPYING HAPPENS WITHIN THEM 

    // Create an init process - pid1
    if (cmd_args[0] != NULL) {

        // NOT DONEEE MAKE SURE TO CHECK STATUS OF LOADPROGRAM AFTER 
        LoadProgram(cmd_args[0], cmd_args, info); 
    } else {

        // NOT DONEEE MAKE SURE TO CHECK STATUS OF LOADPROGRAM AFTER 
        LoadProgram("init", cmd_args, info); 
    }
    char **other_args = &cmd_args[1]; 
    LoadProgram(file_name, other_args, info);
}

// Procedure called by malloc to add more pages to kernel's memory 
int SetKernelBrk(void *addr) {
    int i; 

    // Virtual memory not yet enabled, give more pages in physical memory 
    if (vm_enabled == 0) {
        current_break = addr; 

        return 0; 
    // Virtual memory has been enabled, give more pages in virtual memory 
    } else {

        // check if address is valid (in region 1: above kernel heap and 
        // below allocated memory for page table/context switching)
        if ((addr > current_break) && (addr < (VMEM_1_LIMIT - PAGESIZE * 4))) {

            int curr_top_page = current_break >> PAGESHIFT; 
            int target_page = addr >> PAGESHIFT; 
            int pages = target_page - curr_top_page; 
            
            for (i = 1; i <= pages; i++) {

                int new_page = curr_top_page + i; 
                region1_pt[new_page].valid = VALID;  
                region1_pt[new_page].kprot = PROT_READ | PROT_WRITE; 
                region1_pt[new_page].uprot = PROT_NONE; 

                int new_physical_page =  getFreePage(); 

                // check there is enough physical memory 
                if (new_physical_page != -1) {

                    region1_pt[new_page].pfn = new_physical_page; 
                    free_page_tracker[new_physical_page] = UNFREE; 
                // not enough physical memory 
                } else {

                    return -1; 
                }
            }
            return 0; 
        // invalid address
        } else {

            return -1; 
        }
    }
}

void trap_kernel_handler(ExceptionInfo *info) {

    (void) info;
    TracePrintf(1, "trap kernel handler"); 
    Halt();  
}

void trap_clock_handler(ExceptionInfo *info) {

    (void) info;
    TracePrintf(1, "trap clock handler"); 
    Halt(); 
}

void trap_illegal_handler(ExceptionInfo *info) {

    (void) info;
    TracePrintf(1, "in trap illegal handler"); 
    Halt(); 
}

void trap_memory_handler(ExceptionInfo *info) {

    (void) info;
    TracePrintf(1, "in trap memory handler"); 
    Halt(); 
}

void trap_math_handler(ExceptionInfo *info) {

    (void) info;
    TracePrintf(1, "in trap math handler"); 
    Halt(); 
}

void trap_tty_receive_handler(ExceptionInfo *info) {

    (void) info;
    TracePrintf(1, "in trap tty recieve handler"); 
    Halt(); 
}

void trap_tty_transmit_handler(ExceptionInfo *info) {

    (void) info;
    TracePrintf(1, "in trap tty transmit handler"); 
    Halt(); 
}

/*
 *  Load a program into the current process's address space.  The
 *  program comes from the Unix file identified by "name", and its
 *  arguments come from the array at "args", which is in standard
 *  argv format.
 *
 *  Returns:
 *      0 on success
 *     -1 on any error for which the current process is still runnable
 *     -2 on any error for which the current process is no longer runnable
 *
 *  This function, after a series of initial checks, deletes the
 *  contents of Region 0, thus making the current process no longer
 *  runnable.  Before this point, it is possible to return ERROR
 *  to an Exec() call that has called LoadProgram, and this function
 *  returns -1 for errors up to this point.  After this point, the
 *  contents of Region 0 no longer exist, so the calling user process
 *  is no longer runnable, and this function returns -2 for errors
 *  in this case.
 */
int
LoadProgram(char *name, char **args, ExceptionInfo *info)
{
    int fd;
    int status;
    struct loadinfo li;
    char *cp;
    char *cp2;
    char **cpp;
    char *argbuf;
    int i;
    unsigned long argcount;
    int size;
    int text_npg;
    int data_bss_npg;
    int stack_npg;

    TracePrintf(0, "LoadProgram '%s', args %p\n", name, args);

    if ((fd = open(name, O_RDONLY)) < 0) {
	TracePrintf(0, "LoadProgram: can't open file '%s'\n", name);
	return (-1);
    }

    status = LoadInfo(fd, &li);
    TracePrintf(0, "LoadProgram: LoadInfo status %d\n", status);
    switch (status) {
	case LI_SUCCESS:
	    break;
	case LI_FORMAT_ERROR:
	    TracePrintf(0,
		"LoadProgram: '%s' not in Yalnix format\n", name);
	    close(fd);
	    return (-1);
	case LI_OTHER_ERROR:
	    TracePrintf(0, "LoadProgram: '%s' other error\n", name);
	    close(fd);
	    return (-1);
	default:
	    TracePrintf(0, "LoadProgram: '%s' unknown error\n", name);
	    close(fd);
	    return (-1);
    }
    TracePrintf(0, "text_size 0x%lx, data_size 0x%lx, bss_size 0x%lx\n",
	li.text_size, li.data_size, li.bss_size);
    TracePrintf(0, "entry 0x%lx\n", li.entry);

    /*
     *  Figure out how many bytes are needed to hold the arguments on
     *  the new stack that we are building.  Also count the number of
     *  arguments, to become the argc that the new "main" gets called with.
     */
    size = 0;
    for (i = 0; args[i] != NULL; i++) {
	size += strlen(args[i]) + 1;
    }
    argcount = i;
    TracePrintf(0, "LoadProgram: size %d, argcount %d\n", size, argcount);

    /*
     *  Now save the arguments in a separate buffer in Region 1, since
     *  we are about to delete all of Region 0.
     */
    cp = argbuf = (char *)malloc(size);
    for (i = 0; args[i] != NULL; i++) {
	strcpy(cp, args[i]);
	cp += strlen(cp) + 1;
    }
  
    /*
     *  The arguments will get copied starting at "cp" as set below,
     *  and the argv pointers to the arguments (and the argc value)
     *  will get built starting at "cpp" as set below.  The value for
     *  "cpp" is computed by subtracting off space for the number of
     *  arguments plus 4 (for the argc value, a 0 (AT_NULL) to
     *  terminate the auxiliary vector, a NULL pointer terminating
     *  the argv pointers, and a NULL pointer terminating the envp
     *  pointers) times the size of each (sizeof(void *)).  The
     *  value must also be aligned down to a multiple of 8 boundary.
     */
    cp = ((char *)USER_STACK_LIMIT) - size;
    cpp = (char **)((unsigned long)cp & (-1 << 4));	/* align cpp */
    cpp = (char **)((unsigned long)cpp - ((argcount + 4) * sizeof(void *)));

    text_npg = li.text_size >> PAGESHIFT;
    data_bss_npg = UP_TO_PAGE(li.data_size + li.bss_size) >> PAGESHIFT;
    stack_npg = (USER_STACK_LIMIT - DOWN_TO_PAGE(cpp)) >> PAGESHIFT;

    TracePrintf(0, "LoadProgram: text_npg %d, data_bss_npg %d, stack_npg %d\n",
	text_npg, data_bss_npg, stack_npg);

    /*
     *  Make sure we have enough *virtual* memory to fit everything within
     *  the size of a page table, including leaving at least one page
     *  between the heap and the user stack
     */
    if (MEM_INVALID_PAGES + text_npg + data_bss_npg + 1 + stack_npg +
	1 + KERNEL_STACK_PAGES > PAGE_TABLE_LEN) {
	TracePrintf(0,
	    "LoadProgram: program '%s' size too large for VIRTUAL memory\n",
	    name);
	free(argbuf);
	close(fd);
	return (-1);
    }

    /*
     *  And make sure there will be enough *physical* memory to
     *  load the new program.
     * 
     *  >>>> The new program will require text_npg pages of text,
     *  >>>> data_bss_npg pages of data/bss, and stack_npg pages of
     *  >>>> stack.  In checking that there is enough free physical
     *  >>>> memory for this, be sure to allow for the physical memory
     *  >>>> pages already allocated to this process that will be
     *  >>>> freed below before we allocate the needed pages for
     *  >>>> the new program being loaded.
    */
    int memory_needed = text_npg + data_bss_npg + stack_npg; 

    if (memory_needed < num_free_pages) {
	TracePrintf(0,
	    "LoadProgram: program '%s' size too large for PHYSICAL memory\n",
	    name);
	free(argbuf);
	close(fd);
	return (-1);
    }
    /*
     *  >>>> Initialize sp for the current process to (void *)cpp.
     *  >>>> The value of cpp was initialized above.
    */
    info->sp = (void *)cpp; 

    /*
     *  Free all the old physical memory belonging to this process,
     *  but be sure to leave the kernel stack for this process (which
     *  is also in Region 0) alone.
     
     *  >>>> Loop over all PTEs for the current process's Region 0,
     *  >>>> except for those corresponding to the kernel stack (between
     *  >>>> address KERNEL_STACK_BASE and KERNEL_STACK_LIMIT).  For
     *  >>>> any of these PTEs that are valid, free the physical memory
     *  >>>> memory page indicated by that PTE's pfn field.  Set all
     *  >>>> of these PTEs to be no longer valid.
    */
   
    // >>>> Leave the first MEM_INVALID_PAGES number of PTEs in the
    // >>>> Region 0 page table unused (and thus invalid)
    for (i = 0; i < MEM_INVALID_PAGES; i++) {
        
        // if valid set to invalid and free page 
        if (region0_pt[i].valid == VALID) {

            free_page_tracker[i] = FREE; 
            region0_pt[i].valid = INVALID; 
            region0_pt[i].kprot = PROT_NONE; 
            region0_pt[i].uprot = PROT_NONE; 
            region0_pt[i].pfn = -1; 
            num_free_pages++; 
        }
    }

    /*
     *  Fill in the page table with the right number of text,
     *  data+bss, and stack pages.  We set all the text pages
     *  here to be read/write, just like the data+bss and
     *  stack pages, so that we can read the text into them
     *  from the file.  We then change them read/execute.
     */

    /* First, the text pages */
    // >>>> For the next text_npg number of PTEs in the Region 0
    // >>>> page table, initialize each PTE:
    // >>>>     valid = 1
    // >>>>     kprot = PROT_READ | PROT_WRITE
    // >>>>     uprot = PROT_READ | PROT_EXEC
    // >>>>     pfn   = a new page of physical memory
    int text_pages_limit = MEM_INVALID_PAGES + text_npg; 
    for (i = MEM_INVALID_PAGES; i < text_pages_limit; i++) {

        region0_pt[i].valid = VALID; 
        region0_pt[i].kprot = PROT_READ | PROT_WRITE; 
        region0_pt[i].uprot = PROT_READ | PROT_EXEC; 

        int new_physical_page = getFreePage(); 
        
        // check there is enough physical memory 
        if (new_physical_page != -1) {

            region0_pt[i].pfn = new_physical_page; 
            free_page_tracker[new_physical_page] = UNFREE; 
        // not enough memory 
        } else {

            return -1; 
        }
    }

    /* Then the data and bss pages */
    // >>>> For the next data_bss_npg number of PTEs in the Region 0
    // >>>> page table, initialize each PTE:
    // >>>>     valid = 1
    // >>>>     kprot = PROT_READ | PROT_WRITE
    // >>>>     uprot = PROT_READ | PROT_WRITE
    // >>>>     pfn   = a new page of physical memory
    int data_bss_limit = text_pages_limit + data_bss_npg; 
    for (i = text_pages_limit; i < data_bss_limit; i++) {

        region0_pt[i].valid = VALID; 
        region0_pt[i].kprot = PROT_READ | PROT_WRITE; 
        region0_pt[i].uprot = PROT_READ | PROT_WRITE; 
        
        int new_physical_page = getFreePage(); 
        
        // check there is enough physical memory 
        if (new_physical_page != -1) {

            region0_pt[i].pfn = new_physical_page; 
            free_page_tracker[new_physical_page] = UNFREE; 
        // not enough memory 
        } else {

            return -1; 
        }
    }

    /* And finally the user stack pages */
    // >>>> For stack_npg number of PTEs in the Region 0 page table
    // >>>> corresponding to the user stack (the last page of the
    // >>>> user stack *ends* at virtual address USER_STACK_LIMIT),
    // >>>> initialize each PTE:
    // >>>>     valid = 1
    // >>>>     kprot = PROT_READ | PROT_WRITE
    // >>>>     uprot = PROT_READ | PROT_WRITE
    // >>>>     pfn   = a new page of physical memory
    int user_stack_limit = data_bss_limit + stack_npg; 
    for (i = data_bss_limit; i < user_stack_limit; i++) {

        region0_pt[i].valid = VALID; 
        region0_pt[i].kprot = PROT_READ | PROT_WRITE; 
        region0_pt[i].uprot = PROT_READ | PROT_WRITE; 
        
        int new_physical_page = getFreePage(); 
        
        // check there is enough physical memory 
        if (new_physical_page != -1) {

            region0_pt[i].pfn = new_physical_page; 
            free_page_tracker[new_physical_page] = UNFREE; 
        // not enough memory 
        } else {

            return -1; 
        }
    }

    /*
     *  All pages for the new address space are now in place.  Flush
     *  the TLB to get rid of all the old PTEs from this process, so
     *  we'll be able to do the read() into the new pages below.
     */
    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    /*
     *  Read the text and data from the file into memory.
     */
    if (read(fd, (void *)MEM_INVALID_SIZE, li.text_size+li.data_size)
	!= (int) (li.text_size+li.data_size)){
	TracePrintf(0, "LoadProgram: couldn't read for '%s'\n", name);
	free(argbuf);
	close(fd);
	// >>>> Since we are returning -2 here, this should mean to
	// >>>> the rest of the kernel that the current process should
	// >>>> be terminated with an exit status of ERROR reported
	// >>>> to its parent process.
	return (-2);
    }

    close(fd);			/* we've read it all now */

    /*
     *  Now set the page table entries for the program text to be readable
     *  and executable, but not writable.
     */

    // >>>> For text_npg number of PTEs corresponding to the user text
    // >>>> pages, set each PTE's kprot to PROT_READ | PROT_EXEC.
    for (i = MEM_INVALID_PAGES; i < text_pages_limit; i++) { 

        region0_pt[i].kprot = PROT_READ | PROT_EXEC; 
    }

    WriteRegister(REG_TLB_FLUSH, TLB_FLUSH_0);

    /*
     *  Zero out the bss
     */
    memset((void *)(MEM_INVALID_SIZE + li.text_size + li.data_size),
	'\0', li.bss_size);

    /*
     *  Set the entry point in the ExceptionInfo.
     *  >>>> Initialize pc for the current process to (void *)li.entry
     */
    info->pc = (void *)li.entry; 

    /*
     *  Now, finally, build the argument list on the new stack.
     */
    *cpp++ = (char *)argcount;		/* the first value at cpp is argc */
    cp2 = argbuf;
    for (i = 0; (uint) i < argcount; i++) {      /* copy each argument and set argv */
	*cpp++ = cp;
	strcpy(cp, cp2);
	cp += strlen(cp) + 1;
	cp2 += strlen(cp2) + 1;
    }
    free(argbuf);
    *cpp++ = NULL;	/* the last argv is a NULL pointer */
    *cpp++ = NULL;	/* a NULL pointer for an empty envp */
    *cpp++ = 0;		/* and terminate the auxiliary vector */

    /*
     *  Initialize all regs[] registers for the current process to 0,
     *  initialize the PSR for the current process also to 0.  This
     *  value for the PSR will make the process run in user mode,
     *  since this PSR value of 0 does not have the PSR_MODE bit set.
     * 
     *  Initialize regs[0] through regs[NUM_REGS-1] for the
     *  current process to 0.
     *  Initialize psr for the current process to 0.
     */
    info->psr = 0; 

    for (i = 0; i < NUM_REGS; i++) {

        info->regs[i] = 0; 
    }

    return (0);
}

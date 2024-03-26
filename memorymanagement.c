#include <comp421/yalnix.h>
#include <comp421/hardware.h>

#define VALID   1 
#define INVALID 0

struct free_list_entry {
    unsigned int pfn    : 20;
    struct free_list_entry *next;
};

// Declaration of page tables 
struct pte *region0_pt; 
struct pte *region1_pt; 

// Kernel break
void *curr_kernel_brk = VMEM_1_BASE;

// Boolean to track whether virtual memory has been enabled or not
int vm_enabled = 0;

int num_physical_pages;
int num_free_pages;

// head of the free page list
struct free_list_entry *free_list_head;

int SetKernelBrk(void *addr) {

    // Simple case if VM is not enabled
    if (!vm_enabled) {
        curr_kernel_brk = addr;
    } else {
        // this is much harder.
    }

}

void* getKernelBrk() {
    return curr_kernel_brk;
}

// void initFreePagesList(unsigned int pmem_size) {

//     // Find the already used pages
//     num_physical_pages = pmem_size / PAGESIZE;
//     num_free_pages = 0;

//     int used_pages_min = DOWN_TO_PAGE(VMEM_1_BASE) / PAGESIZE;
//     int used_pages_max = UP_TO_PAGE(curr_kernel_brk) / PAGESIZE;

//     /* Create the free list*/

//     // Head of the free list
//     free_list_head = Malloc(sizeof(struct free_list_entry*));
//     free_list_head->pfn = 0;
//     free_list_head->next = NULL;

//     // Make the rest of the free list
//     for (int i = MEM_INVALID_PAGES; i < num_physical_pages; i++) {
        
//         if (i < used_pages_min || i > used_pages_max) {
//             // The page is free. Add it to the free list

//             struct free_list_entry *new_pfn_entry = malloc(sizeof (struct free_list_entry*));
//             if (new_pfn_entry == NULL) {
//                 errorFunc();
//             }
//             new_pfn_entry->pfn = i;
//             new_pfn_entry->next = free_list_head;
//             free_list_head = new_pfn_entry;
//             num_free_pages++;
//         }
//     }
// } 

void initPageTables() {
    region0_pt = VMEM_1_LIMIT - PAGESIZE; 
    region1_pt = VMEM_1_LIMIT - (PAGESIZE * 2);

    for (int i = 0; i < VMEM_LIMIT >> PAGESHIFT; i++) {

        // Before Region 0 kernel stack in memory 
        if (i < KERNEL_STACK_BASE >> PAGESHIFT) {
            region0_pt[i].pfn = i; 
            region0_pt[i].kprot = PROT_NONE; 
            region0_pt[i].uprot = PROT_NONE; 
            region0_pt[i].valid = INVALID; 

        // In Region 0 kernel stack in memory 
        } else if (i < KERNEL_STACK_LIMIT >> PAGESHIFT) {
            region0_pt[i].pfn = i; 
            region0_pt[i].kprot = PROT_READ | PROT_WRITE; 
            region0_pt[i].uprot = PROT_NONE; 
            region0_pt[i].valid = VALID; 

        // In Region 1 kernel text pages of kernel heap in memory 
        } else if (i < &_etext >> PAGESHIFT) {
            region1_pt[i].pfn = i; 
            region1_pt[i].kprot = PROT_EXEC; 
            region1_pt[i].uprot = PROT_NONE; 
            region1_pt[i].valid = VALID; 

        // In Region 1 kernel data/bss/heap pages of kernel heap in memory 
        } else if (i < current_break >> PAGESHIFT) {
            region1_pt[i].pfn = i; 
            region1_pt[i].kprot = PROT_READ | PROT_WRITE; 
            region1_pt[i].uprot = PROT_NONE; 
            region1_pt[i].valid = VALID; 

        // In Region 1 space under our allocated data structures in memory 
        } else if (i < VMEM_1_LIMIT - (PAGESIZE * 2)) {
            region1_pt[i].pfn = i; 
            region1_pt[i].kprot = PROT_NONE; 
            region1_pt[i].uprot = PROT_NONE; 
            region1_pt[i].valid = INVALID; 

        // In Region 1 space with our allocated data stuctures in memory 
        } else {
            region1_pt[i].pfn = i; 
            region1_pt[i].kprot = PROT_READ | PROT_WRITE; 
            region1_pt[i].uprot = PROT_NONE; 
            region1_pt[i].valid = VALID; 
        }
    }

    // Initialize the page table registers to point to their respective page tables 
    WriteRegister(REG_PTR0, (RCS421RegVal) region0_pt); 
    WriteRegister(REG_PTR1, (RCS421RegVal) region1_pt); 
}

void enable_VM() {
    WriteRegister(REG_VM_ENABLE, 1);
    vm_enabled = 1;
}


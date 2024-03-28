#include <comp421/yalnix.h>
#include <comp421/hardware.h>

#include "memorymanagement.h"

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

int num_physical_pages;

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

/** 
 * This function obtains a free page from the free page list 
 * */

int GetFreePage () {

    // Gets the first available PFN from the list
    int pfn = free_head;

    // Borrow the page table entry from the top of the region 1 page table
    BorrowPTE();
    region1_pt[borrowed_idx].pfn = pfn;

    // Move to the next page in the list
    unsigned int *addr = (unsigned int*) borrowed_addr;
    free_head = *addr;

    // Decrements the number of free pages
    free_num_pages--;

    // Frees the borrowed PTE
    ReleasePTE();

    // Returns the PFN
    return pfn;
}

/* Adds a page to the free page list */
void AddFreePage (int index, int pfn) {

    // Computes the virtual address of the page
    unsigned int *addr = (unsigned int*)((uintptr_t) VMEM_0_BASE + index * PAGESIZE);

    // Flushes the address from the TLB
    WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) addr);

    // Adds the page to the list
    *addr = free_head;
    free_head = pfn;

    // Increments the number of free pages
    num_free_pages++;
}

/**
 * PAGE TABLE HANDLING
*/

// void initPageTables() {
//     region0_pt = VMEM_1_LIMIT - PAGESIZE; 
//     region1_pt = VMEM_1_LIMIT - (PAGESIZE * 2);

//     for (int i = 0; i < VMEM_LIMIT >> PAGESHIFT; i++) {

//         // Before Region 0 kernel stack in memory 
//         if (i < KERNEL_STACK_BASE >> PAGESHIFT) {
//             region0_pt[i].pfn = i; 
//             region0_pt[i].kprot = PROT_NONE; 
//             region0_pt[i].uprot = PROT_NONE; 
//             region0_pt[i].valid = INVALID; 

//         // In Region 0 kernel stack in memory 
//         } else if (i < KERNEL_STACK_LIMIT >> PAGESHIFT) {
//             region0_pt[i].pfn = i; 
//             region0_pt[i].kprot = PROT_READ | PROT_WRITE; 
//             region0_pt[i].uprot = PROT_NONE; 
//             region0_pt[i].valid = VALID; 

//         // In Region 1 kernel text pages of kernel heap in memory 
//         } else if (i < &_etext >> PAGESHIFT) {
//             region1_pt[i].pfn = i; 
//             region1_pt[i].kprot = PROT_EXEC; 
//             region1_pt[i].uprot = PROT_NONE; 
//             region1_pt[i].valid = VALID; 

//         // In Region 1 kernel data/bss/heap pages of kernel heap in memory 
//         } else if (i < current_break >> PAGESHIFT) {
//             region1_pt[i].pfn = i; 
//             region1_pt[i].kprot = PROT_READ | PROT_WRITE; 
//             region1_pt[i].uprot = PROT_NONE; 
//             region1_pt[i].valid = VALID; 

//         // In Region 1 space under our allocated data structures in memory 
//         } else if (i < VMEM_1_LIMIT - (PAGESIZE * 2)) {
//             region1_pt[i].pfn = i; 
//             region1_pt[i].kprot = PROT_NONE; 
//             region1_pt[i].uprot = PROT_NONE; 
//             region1_pt[i].valid = INVALID; 

//         // In Region 1 space with our allocated data stuctures in memory 
//         } else {
//             region1_pt[i].pfn = i; 
//             region1_pt[i].kprot = PROT_READ | PROT_WRITE; 
//             region1_pt[i].uprot = PROT_NONE; 
//             region1_pt[i].valid = VALID; 
//         }
//     }

//     // Initialize the page table registers to point to their respective page tables 
//     WriteRegister(REG_PTR0, (RCS421RegVal) region0_pt); 
//     WriteRegister(REG_PTR1, (RCS421RegVal) region1_pt); 
// }

// void enable_VM() {
//     WriteRegister(REG_VM_ENABLE, 1);
//     vm_enabled = 1;
// }

/* Borrows a new PTE from the region 1 page table */
void BorrowPTE () {

    // Updates the borrowed address and index
    borrowed_addr = (void*)((uintptr_t) borrowed_addr - PAGESIZE);
    borrowed_idx--;

    // Temporarily buffers the current PTE if in use
    if(region1_pt[borrowed_idx].valid) {
        pte_buffer[pte_count] = region1_pt[borrowed_idx];
        pte_count++;

        // Flushes the invalidated TLB entry
        WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) borrowed_addr);
    }

    // Initializes the PTE
    region1_pt[borrowed_idx].valid = 1;
    region1_pt[borrowed_idx].kprot = PROT_READ | PROT_WRITE;
}


/* Frees a PTE borrowed from the region 1 page table */
void ReleasePTE () {

    // Frees the PTE
    pt1[borrowed_idx].valid = 0;

    // Restores a buffered PTE
    if(pte_count) {
        pte_count--;
        pt1[borrowed_idx] = pte_buffer[pte_count];
    }
    WriteRegister(REG_TLB_FLUSH, (RCS421RegVal) borrowed_addr);

    // Updates the borrowed address and index
    borrowed_addr = (void*)((uintptr_t) borrowed_addr + PAGESIZE);
    borrowed_idx++;
}

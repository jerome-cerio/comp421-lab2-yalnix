#include <comp421/yalnix.h>
#include <comp421/hardware.h>

struct free_list_entry {
    unsigned int pfn    : 20;
    struct free_list_entry *next;
};

void *curr_kernel_brk = VMEM_1_BASE;
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

void initFreePagesList(unsigned int pmem_size) {

    // Find the already used pages
    num_physical_pages = pmem_size / PAGESIZE;
    num_free_pages = 0;

    int used_pages_min = DOWN_TO_PAGE(VMEM_1_BASE) / PAGESIZE;
    int used_pages_max = UP_TO_PAGE(curr_kernel_brk) / PAGESIZE;

    /* Create the free list*/

    // Head of the free list
    free_list_head = Malloc(sizeof(struct free_list_entry*));
    free_list_head->pfn = 0;
    free_list_head->next = NULL;

    // Make the rest of the free list
    for (int i = MEM_INVALID_PAGES; i < num_physical_pages; i++) {
        
        if (i < used_pages_min || i > used_pages_max) {
            // The page is free. Add it to the free list

            struct free_list_entry *new_pfn_entry = malloc(sizeof (struct free_list_entry*));
            if (new_pfn_entry == NULL) {
                errorFunc();
            }
            new_pfn_entry->pfn = i;
            new_pfn_entry->next = free_list_head;
            free_list_head = new_pfn_entry;
            num_free_pages++;
        }
    }
} 

void initReg1PageTable() {

}

void initReg0PageTable() {

}

void enable_VM() {
    WriteRegister(REG_VM_ENABLE, 1);
    vm_enabled = 1;
}


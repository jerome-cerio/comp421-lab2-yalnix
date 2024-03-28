#ifndef MEMORYMANAGEMENT_H
#define MEMORYMANAGEMENT_H

#include <comp421/hardware.h>
#include <stdint.h>

// Boolean to track whether virtual memory has been enabled or not
int vm_enabled = 0;

// Manages a list of free pages
unsigned int free_head;
unsigned int num_free_pages;

// Manages free page tables
int *free_tables;
unsigned int num_free_tables;

// Stores the address of the region 1 page table
uintptr_t ptaddr1;
//struct pte *pt1;

// Stores the address and index of a borrowed PTE
void *borrowed_addr;
int borrowed_idx;

// Temporarily buffers borrowed PTEs if required
struct pte pte_buffer[4];
int pte_count;


// Function prototypes
int GetFreePage ();
void AddFreePage (int , int );

void BorrowPTE ();
void ReleasePTE ();

//uintptr_t GetPageTable ();
void FreePageTable (uintptr_t );

void CopyKernelStack (uintptr_t );

#endif
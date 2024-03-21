#ifndef QUEUES_H
#define QUEUES_H

#include <comp421/hardware.h>
#include <comp421/yalnix.h>
#include <stdint.h>

// Struct for a queue of process control blocks.
struct pcb_queue {
    struct pcb_node *head;
    struct pcb_node *tail;
    int size;
};

// Queues for the "ready" and "blocked" queues.
struct pcb_queue ready;
struct pcb_queue blocked;

void enqueue (struct pcb_queue*, struct pcb_node*);
struct pcb_node* dequeue(struct pcb_queue*)

#endif
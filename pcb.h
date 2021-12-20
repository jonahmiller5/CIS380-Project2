#ifndef _PCB_H
#define _PCB_H

#include <string.h>
#include <stdlib.h> 
#include <stdio.h>
#include <ucontext.h>
#include "fs/descriptors.h"
#include <valgrind/valgrind.h>


// note that ZOMBIE and KILLED both mean that a process has terminated. 
// the difference is that ZOMBIE means the process terminated naturally and KILLED is due to an external signal
#define TRUE 1
#define FALSE 0
#define RUNNING 0
#define BLOCKED 1
#define FINISHED 2
#define READY 3
#define ZOMBIE 4
#define ORPHAN 5
#define KILLED 6
#define STOPPED 7
#define READ 0
#define WRITE 1
#define BG 0
#define FG 1

typedef struct pcb {
    struct pcb * parent_pcb;
    int pgid;
    int ppid;
    int pid;
    ucontext_t * context;
    int priority;
    char * command; // unsure if necessary
    int bool_read;
    int bool_write;
    int status;
    // equals 1 if process terminated naturally, 0 otherwise; value is set in scheduler
    int terminated_natty;
    int file_descriptor;
    // for now not including file descriptors
    // connection to all other PCBS? --> keep global LL of them?
    // maybe do seprate impl of ll from
    struct pcb * next;
    int child_changed;
    struct pcb ** child_list;
    int num_children;
    int child_list_size;
    int changed;
    int sleep_time;
    FD_LIST* fd_list;   // the file descriptor list
    int ground;         // is the process allowed to read from STDIN ?
    int signal_flag;
} pcb;

// Context, pid, pgid, ppid, priority, file desciptors (?), file permissions, status (zombie/orphan/running/blocked/ready), maybe don't need orphan?
void set_ground(pcb * block, int new_ground);
pcb * create_PCB (int p, int pg, int pp, ucontext_t * con, int b_read, int b_write, int prior);
pcb * find_next_valid(pcb * head);
pcb * add_PCB (pcb* head, pcb* newBlock);
pcb * add_to_head(pcb* old_head, pcb* new_head);
pcb * remove_PCB (pcb* head, pcb* remBlock);
pcb * find_PCB_pid(pcb* head, int p);
pcb * find_PCB_pgid(pcb* head, int p);
pcb * find_PCB_ppid(pcb* head, int p);
void add_child_PCB(pcb* block, pcb* child);
void free_PCB(pcb * block);
void print_PCB(pcb * current);
void set_sleep(pcb * block, int sleep_time);
void set_command(pcb * block, char * command);
void remove_child_PCB(pcb * proc);

#endif
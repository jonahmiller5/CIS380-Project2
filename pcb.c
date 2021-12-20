#include <ucontext.h>
#include "pcb.h"



// NOTES ABOUT PCB
// In PCB: Context, pid, pgid, ppid, priority, file desciptors (?), file permissions, status (zombie/orphan/running/blocked/ready)
// Kill all children/grandchildren if current PCB dies/zombifies

pcb * create_PCB (int p, int pg, int pp, ucontext_t * con, int b_read, int b_write, int prior){
    pcb * new_pcb;
    new_pcb = (pcb*) malloc(sizeof(pcb));
    // unsure if we are adding command/ action associated with specific PCB

    // new field -- pointer to parent pcb
    new_pcb->parent_pcb = NULL;

    new_pcb->pid = p;
    new_pcb->pgid = pg;
    new_pcb->ppid = pp;
    new_pcb->context = con;
    new_pcb->bool_read = b_read;
    new_pcb->bool_write = b_write;
    new_pcb->priority = prior;
    new_pcb->changed = FALSE;
    new_pcb->status = READY; // <-- if just created is it running or blocked?
    new_pcb->terminated_natty = 0;
    new_pcb->child_list = (pcb **)malloc(2 * sizeof(pcb*));
    memset(new_pcb->child_list, '\0', 2 * sizeof(pcb*));
    // new_pcb->child_list = malloc(2 * sizeof(pcb*));
    new_pcb->num_children = 0;
    new_pcb->child_list_size = 2;
    new_pcb->sleep_time = -1;
    new_pcb->fd_list = init_fd_list(NULL);
    new_pcb->ground = FG;
    new_pcb->signal_flag = 0;

    return new_pcb;
}

// NOTE: figure out when the fuck ground type gets set (prolly in shell)
void set_ground(pcb * block, int new_ground) {
    if (block == NULL || (new_ground != FG && new_ground != BG)) return;
    block->ground = new_ground;
}

pcb * add_PCB (pcb* head, pcb* new_block){
    if (head == NULL && new_block == NULL){
        return head;
    }
    if (head == NULL){
        head = new_block;
        return head;
    }
    if (head == new_block) {
        return head;
    }

    // check if new_block is in head
    pcb * current = head;
    while (current->next != NULL){
        if (current->pid == new_block->pid) {
            return head;
        }
        current = current->next;
    }


    current = head;
    while (current->next != NULL){
        current = current->next;
    }
    current->next = new_block;

    //added to debug infinite loop
    new_block->next = NULL;

    return head;
}

// ONLY TO BE CALLED IN KILL
pcb * add_to_head(pcb* old_head, pcb* new_head) {

    // need to add back to parent if we reenque
    if (new_head->parent_pcb != NULL){
        add_child_PCB(new_head->parent_pcb, new_head);
    }
    if (new_head == NULL) return old_head;

    if (old_head == NULL) return new_head;

    new_head->next = old_head;

    return new_head;
}


void remove_child_PCB(pcb * proc){
    if (proc == NULL){
        return;
    }
    pcb * parent =  proc->parent_pcb;
    //
    int found_child = 0;
    int i = 0;
    if (parent != NULL) {
        for (i = 0; i < parent->num_children; i++) {
            if (parent->child_list[i]->pid == proc->pid) {
                found_child = 1;
                parent->child_list[i] = NULL;
            }
            // move every pcb after child over by 1 index
            if (found_child == 1 && i != parent->num_children - 1) {
                parent->child_list[i] = parent->child_list[i+1];
            }
        }
        if (found_child == 1){
            parent->child_list[parent->num_children] = NULL;
            parent->num_children -= 1;
        }

    }
}

// TODO: create function to add a new child
pcb* remove_PCB (pcb* head, pcb* rem_block){
    if (rem_block == NULL || head == NULL){
        return head;
    }

    if (find_PCB_pid(head, rem_block->pid) == NULL) {
        return head;
    }

    // remove PCB from parent's child_list
    // use PCB ppid in find by pid to get parent

    // Note: do we necessarily want to do this, since removing a PCB is part of executing it? Just a thought.
    // int parent_pid = rem_block->ppid;

    //printf("remove top\n");

    pcb * parent =  rem_block->parent_pcb;
    //
    int found_child = 0;
    int i = 0;
    if (parent != NULL) {
        for (i = 0; i < parent->num_children; i++) {
            if (parent->child_list[i]->pid == rem_block->pid) {
                found_child = 1;
                parent->child_list[i] = NULL;
            }
            // move every pcb after child over by 1 index
            if (found_child == 1 && i != parent->num_children - 1) {
                parent->child_list[i] = parent->child_list[i+1];
            }
        }
        if (found_child == 1){
            parent->child_list[parent->num_children] = NULL;
            parent->num_children -= 1;
        }

    }
    //printf("remove bottom\n");
    if (head == rem_block){
        // print_PCB(head);
        pcb* temp = head;
        head = temp->next;
        temp->next = NULL;
        return head;

    }

    pcb * previous = head;
    pcb * current = head->next;
    while (current != NULL) {
        if (current == rem_block){
            previous->next = current->next;
            return head;
        }
        previous = current;
        current = current->next;
    }


    // added to debug infinite loop
    rem_block->next = NULL;

    return head;
}

pcb * find_next_valid(pcb * head) {
    while (head != NULL && head->status != READY) {
        head = head->next;
    }
    return head;
}

//searching through children done by jonah;
pcb * find_PCB_pid(pcb* head, int p){
    pcb * current = head;
    while (current != NULL){
        if (current->pid == p){
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// should find all pcb with same pgid
pcb * find_PCB_pgid(pcb* head, int p){
    pcb * current = head;
    while (current != NULL){
        if (current->pgid == p){
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// find all pcb with same parent --> use child wrapper
pcb * find_PCB_ppid(pcb* head, int p){
    pcb * current = head;
    while (current != NULL){
        if (current->ppid == p){
            return current;
        }
        current = current->next;
    }
    return NULL;
}

// add childs to the child_list of a pcb and increments num_children
void add_child_PCB(pcb * block, pcb * child) {
    //checks to see if list has it
    for (int i = 0; i < block->num_children; i++){
        if (block->child_list[i]->pid == child->pid){
            return;
        }
    }
    // if our child_list array is at capacity, double size of the list and copy the elements
    if (block->num_children == block->child_list_size - 1) {
        pcb ** temp_list = block->child_list;
        block->child_list = (pcb **)malloc(sizeof(temp_list) * 2 * sizeof(pcb*));
        memset(block->child_list, '\0', sizeof(temp_list) * 2 * sizeof(pcb*));
        block->child_list_size *= 2;
        for (int i = 0; i < block->num_children; i++) {
            block->child_list[i] = temp_list[i];
        }
        free(temp_list);
    }
    child->parent_pcb = block;

    block->child_list[block->num_children] = child;
    block->num_children ++;
}

void free_PCB( pcb * block){
    //free_fd_list(block->fd_list);
    free(block->child_list);
    VALGRIND_STACK_DEREGISTER(block->context->uc_stack.ss_sp);
    free(block);
}


void print_PCB(pcb * current){
    printf("PID: %d\n",current->pid);
    printf("PPID: %d\n",current->ppid);
    printf("PGID: %d\n",current->pgid);
    if (current->next != NULL){
        printf("has_Next: TRUE\n");
    } else{
        printf("has_Next: FALSE\n");
    }

    if (current->context != NULL){
        printf("context: NON NULL\n");
    } else{
        printf("context: NULL\n");
    }
    printf("Priority: %d\n", current->priority );
    printf("Write: %d\n", current->bool_write );
    printf("Read: %d\n", current->bool_read );
    printf("File desciptor: %d\n", current->file_descriptor);
    
}

void set_sleep(pcb * block, int sleep_time) {
    if (block == NULL || sleep_time <= 0) return;
    block->status = BLOCKED;
    block->sleep_time = sleep_time;
}

void set_command(pcb * block, char * command) {
    if (block == NULL || command == NULL) return;

    block->command = command;
    //block->command = "dummy command";
}
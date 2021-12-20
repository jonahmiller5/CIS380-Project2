#include "user_functions.h"
#include "fs/filesystem.h"


p_wait_struct * create_p_wait_struct(int pid, int status) {
	p_wait_struct * new_wait_struct = malloc(sizeof(create_p_wait_struct));

	new_wait_struct->pid = pid;
	new_wait_struct->status = status;

	return new_wait_struct;
}

// ------   Process Status Functions   ------

// return true if the child terminated normally, that is, by a call to p_exit or by returning.
int W_WIFEXITED(int status) {
	return status == ZOMBIE;
}


// return true if the child was stopped by a signal.
int W_WIFSTOPPED(int status) {
	return status == STOPPED;
}


// return true if the child was continued by a signal.
int W_WIFCONTINUED(int status) {
	return status == READY;
}


// return true if the child was terminated by a signal, that is, by a call to p_kill with the S_SIGTERM signal.
int W_WIFSIGNALED(int status) {
	return status == KILLED; 
}

// ------   Process Status Functions   ------

// The context created needs to be passed into mkcontext() in scheduler.c
// returns pid of new thread.
int p_spawn(void * func, int argc, char const *argv[], int ground) {
	// Will fork a new thread that retains most of the attributes of the parent thread (see k_process_create)
	// Once the thread is spawned, it will execute the function referenced by func
	// Create new pcb and then pass it to k_process_create()
	ucontext_t * child_context = malloc(sizeof(ucontext_t));
	mkcontext(child_context, func, argc, argv); // eventually will need more arguments
	pcb * parent = k_get_current_process();
	if (parent == NULL && !strcmp(argv[0], "shell")) {
		parent = get_prev_pcb();
	}

	pcb * new_process = k_process_create(parent, child_context);

	new_process -> ground = ground;

	if (argv != NULL) {
		set_command(new_process,argv[0]);
	} else {
		set_command(new_process, "SHELL");
	}

	// log the creation of the process
	fprintf(get_fptr(), "[%d]\t\tCREATE\t\t\t%d\t\t%d\t\t%s\n", get_timer_total(), new_process->pid, new_process->priority, new_process->command);

	return new_process->pid;
}

void p_kill(int pid, int signal) {
	// Kill the thread referenced by pid with the signal "signal"
	// Find the pcb with the associated pid and pass it to k_process_kill()

	//if signal equals sigterm 
	pcb * process = k_get_process_pid(pid);
	k_process_kill(process, signal);
}

// We need to keep track of this to free it
p_wait_struct * p_wait(int mode, int given_pid) {
	pcb * pcb_calling;
	if (mode == HANG) {
		pcb_calling = get_prev_fg_pcb();
	}
	else {
		pcb_calling = get_prev_pcb();
	}
	
	p_wait_struct * pws = NULL;
	// check children 
	
	if (pcb_calling->num_children == 0){
		return pws;
	}
	// wait on any child
	if (given_pid < 0){
		if (mode == NOHANG) {
			for (int i = 0; i < pcb_calling->num_children; i ++){
				if (pcb_calling->child_list[i]->changed == TRUE){

					// log waited
					fprintf(get_fptr(), "[%d]\t\tWAITED\t\t\t%d\t\t%d\t\t%s\n", get_timer_total(), pcb_calling->child_list[i]->pid, pcb_calling->child_list[i]->priority, pcb_calling->child_list[i]->command);

					if (pcb_calling->child_list[i]->status == ZOMBIE || pcb_calling->child_list[i]->status == KILLED){
						// set wifexited

						pws = create_p_wait_struct(pcb_calling->child_list[i]->pid, pcb_calling->child_list[i]->status);
						k_process_terminate(pcb_calling->child_list[i]);
					} else{

						k_process_update_changed(pcb_calling->child_list[i],FALSE);
						pws = create_p_wait_struct(pcb_calling->child_list[i]->pid, pcb_calling->child_list[i]->status);
					}
					return pws;
				}

			}
			// PWS should be null because no hits on children to be waited on
			return pws;
		}else{
			
			// already have a changed child so check it 
			for (int i = 0; i < pcb_calling->num_children; i ++){
				if (pcb_calling->child_list[i]->changed == TRUE){

					fprintf(get_fptr(), "[%d]\t\tWAITED\t\t\t%d\t\t%d\t\t%s\n", get_timer_total(), pcb_calling->child_list[i]->pid, pcb_calling->child_list[i]->priority, pcb_calling->child_list[i]->command);

					if (pcb_calling->child_list[i]->status == ZOMBIE || pcb_calling->child_list[i]->status == KILLED){
						// set wifexited
						pws = create_p_wait_struct(pcb_calling->child_list[i]->pid, pcb_calling->child_list[i]->status);
						k_process_terminate(pcb_calling->child_list[i]);
					} else{
						k_process_update_changed(pcb_calling->child_list[i],FALSE);
						pws = create_p_wait_struct(pcb_calling->child_list[i]->pid, pcb_calling->child_list[i]->status);
					}
					return pws;
				}
			}
			k_process_swap_to_scheduler();
			// return when context switched back --> this is done is in sigkill

			for (int i = 0; i < pcb_calling->num_children; i ++){
				if (pcb_calling->child_list[i]->changed == TRUE){
					fprintf(get_fptr(), "[%d]\t\tWAITED\t\t\t%d\t\t%d\t\t%s\n", get_timer_total(), pcb_calling->child_list[i]->pid, pcb_calling->child_list[i]->priority, pcb_calling->child_list[i]->command);

					if (pcb_calling->child_list[i]->status == ZOMBIE || pcb_calling->child_list[i]->status == KILLED){
						// set wifexited
						pws = create_p_wait_struct(pcb_calling->child_list[i]->pid, pcb_calling->child_list[i]->status);
						k_process_terminate(pcb_calling->child_list[i]);
					} else{
						// if child status cont or stopped
						k_process_update_changed(pcb_calling->child_list[i],FALSE);
						pws = create_p_wait_struct(pcb_calling->child_list[i]->pid, pcb_calling->child_list[i]->status);
					}
					// should always hit
					return pws;
				}
			}
			// should not really hit
			return NULL;
		}
	// specified pid given to wait on
	} else {
		if (mode == NOHANG) {
			for (int i = 0; i < pcb_calling->num_children; i ++){
				if (pcb_calling->child_list[i]->changed == TRUE  && pcb_calling->child_list[i]->pid == given_pid){

					// log waited
					fprintf(get_fptr(), "[%d]\t\tWAITED\t\t\t%d\t\t%d\t\t%s\n", get_timer_total(), pcb_calling->child_list[i]->pid, pcb_calling->child_list[i]->priority, pcb_calling->child_list[i]->command);

					if (pcb_calling->child_list[i]->status == ZOMBIE || pcb_calling->child_list[i]->status == KILLED){

						// set wifexited
						pws = create_p_wait_struct(pcb_calling->child_list[i]->pid, pcb_calling->child_list[i]->status);
						k_process_terminate(pcb_calling->child_list[i]);
					} else{

						k_process_update_changed(pcb_calling->child_list[i],FALSE);
						pws = create_p_wait_struct(pcb_calling->child_list[i]->pid, pcb_calling->child_list[i]->status);
					}
					return pws;
				}

			}
			// PWS should be null because no hits on children to be waited on
			return pws;
		} else {
			// wait for correct process to finish
			int correct_change = FALSE;
			while (correct_change == FALSE){
				for (int i = 0; i < pcb_calling->num_children; i ++){
					if (pcb_calling->child_list[i]->changed == TRUE && pcb_calling->child_list[i]->pid == given_pid){
						fprintf(get_fptr(), "[%d]\t\tWAITED\t\t\t%d\t\t%d\t\t%s\n", get_timer_total(), pcb_calling->child_list[i]->pid, pcb_calling->child_list[i]->priority, pcb_calling->child_list[i]->command);
						if (pcb_calling->child_list[i]->status == ZOMBIE || pcb_calling->child_list[i]->status == KILLED){
							// set wifexited
							pws = create_p_wait_struct(pcb_calling->child_list[i]->pid, pcb_calling->child_list[i]->status);
							k_process_terminate(pcb_calling->child_list[i]);

						} else{
							k_process_update_changed(pcb_calling->child_list[i],FALSE);
							pws = create_p_wait_struct(pcb_calling->child_list[i]->pid, pcb_calling->child_list[i]->status);
						}
						correct_change = TRUE;
						return pws;
					}
				}
				k_process_swap_to_scheduler();
			}
			// should not really hit
			return NULL;
		}
	}
	
}

// make functionn pid return from kernel current pcb


void p_exit() {
	// set WIF EXITED
	pcb * current = k_get_current_process();
	//log exit
	k_process_terminate(current);

}

void p_nice(int pid, int priority){
	if (priority < 2 && priority > -2 && pid > 0){
		k_process_nice(pid, priority);
	}
}

void empty() {
}

void _sleep(char * t_str) {
	int ticks = atoi(t_str);

	if (ticks <= 0) {
		return;
	}

	p_sleep(ticks);
}


void p_sleep(int ticks) {
	pcb * s_pcb = get_prev_pcb();
	set_sleep(s_pcb, ticks * 10);
	// s_pcb->ground = BG;
	switch(s_pcb->priority) {
		case -1:
			set_neg(add_PCB(get_neg(), s_pcb));
			break;
		case 0:
			set_zero(add_PCB(get_zero(), s_pcb));
			break;
		case 1:
			set_one(add_PCB(get_one(), s_pcb));
			break;
	}

	swapcontext(s_pcb->context, get_scheduler_context());
}

info * p_info(int pid) {
	return k_info(pid);
}

// ------ ZOMBIFY ------
void zombie_child() {
	return;
}

void zombify() {
	char * cmds1[1] = {"zombie child1"};
	// char * cmds2[1] = {"zombie child2"};
	// char * cmds3[1] = {"zombie child3"};
	// char * cmds4[1] = {"zombie child4"};
	p_spawn(zombie_child, 1, cmds1, 1);
	// p_spawn(zombie_child, 1, cmds2, 1);
	// p_spawn(zombie_child, 1, cmds3, 1);
	// p_spawn(zombie_child, 1, cmds4, 1);

	while (1) {

	}
	return;
}

// ------ ORPHANIFY ------
void orphan_child() {
	while (1) {

	}
}

void orphanify() {
	char * cmds[1] = {"orphan_child"};
	p_spawn(orphan_child, 1, cmds, 1);
	return;
}

void p_end_everything() {
	 k_end_everything();
}

int p_pid_present(int pid) {
	if (find_PCB_pid(get_neg(), pid) == NULL) {
		if (find_PCB_pid(get_zero(), pid) == NULL) {
			if (find_PCB_pid(get_one(), pid) == NULL) {
				return FALSE;
			}
		}
	}
	return TRUE;
}

FD_LIST * p_get_fd_list(int pid) {
if (pid < 1) return NULL;

pcb * find = find_PCB_pid(get_neg(), pid);
	if (find == NULL) find = find_PCB_pid(get_zero(), pid);
	if (find == NULL) find = find_PCB_pid(get_one(), pid);
	if (find == NULL) return NULL;

	if (find == NULL || find->fd_list == NULL) return NULL; 

	return find->fd_list;
}

void p_set_fd_list(FD_LIST * list, int pid) {
	if (pid < 1 || list == NULL) return;

	pcb * find = find_PCB_pid(get_neg(), pid);
	if (find == NULL) find = find_PCB_pid(get_zero(), pid);
	if (find == NULL) find = find_PCB_pid(get_one(), pid);
	if (find == NULL) return;

	find->fd_list = list;
}
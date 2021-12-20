#include "kernel_functions.h"

// global pid
int pid_count = 1;
pcb * priority_queue_neg;
pcb * priority_queue_zero;
pcb * priority_queue_one;
pcb * cur_pcb;
pcb * shell_pcb;
ucontext_t * scheduler_context;
void * scheduler_stack;
int timer_total = 0;
FILE * fptr;
int end_everything = FALSE;
int current_queue = 0;

sigset_t signal_mask; // signal mask, applies to all processes
struct itimerval it; // time interval, applies to all processes
void * signal_stack; //global interrupt stack 
ucontext_t * signal_context; //the interrupt context 
ucontext_t * idle_context;

int timer_mod = 0;
int how_finished = TIMEOUT;

// new: this is for the demo where we will need to save informatino about zombies 
pcb * zombie_queue;

// prev_fg: tracks the current fb pcb or previous pcb (if the current PCB is NULL, i.e. scheduler is in idle process)
pcb * prev_fg = NULL;
// same as prev_fg but includes background processes
pcb * prev_cur = NULL;

pcb * get_prev_fg_pcb() {
	return prev_fg;
}

pcb * get_prev_pcb() {
	return prev_cur;
}

void set_shell_pcb(pcb * p){
	shell_pcb = p;
}

pcb * get_shell_pcb(){
	return shell_pcb;
}

void set_prev_pcb(pcb * new_prev) {
	prev_cur = new_prev;
}

void set_prev_fg_pcb(pcb * new_prev) {
	prev_fg = new_prev;
}

pcb * get_neg() {
	return priority_queue_neg;
}

void set_neg(pcb * neg) {
	priority_queue_neg = neg;
}

pcb * get_zero() {
	return priority_queue_zero;
}

void set_zero(pcb * zero) {
	priority_queue_zero = zero;
}

pcb * get_one() {
	return priority_queue_one;
}

void set_one(pcb * one) {
	priority_queue_one = one;
}

pcb * get_zombies() {
	return zombie_queue;
}

pcb * get_cur_pcb() {
	return cur_pcb;
}

void set_cur_pcb(pcb * new_pcb) {
	cur_pcb = new_pcb;
}

ucontext_t * get_scheduler_context() {
	return scheduler_context;
}

void reset_scheduler() {
	getcontext(scheduler_context);
	scheduler_context->uc_stack.ss_sp = scheduler_stack;
    scheduler_context->uc_stack.ss_size = STACKSIZE;
    scheduler_context->uc_stack.ss_flags = 0;
    sigemptyset(&(scheduler_context->uc_sigmask));
    sigaddset(&(scheduler_context->uc_sigmask), SIGALRM);
    makecontext(scheduler_context, scheduler, 1);
}

void setup_scheduler() {
	scheduler_context = malloc(sizeof(ucontext_t));
	scheduler_stack = malloc(STACKSIZE);
	VALGRIND_STACK_REGISTER(scheduler_stack, scheduler_stack + STACKSIZE);
	reset_scheduler();
}

int get_timer_total() {
	return timer_total;
}

void incr_timer() {
	timer_total++;
}

FILE * get_fptr() {
	return fptr;
}

void set_fptr(FILE * f) {
	fptr = f;
}

void fclose_fptr() {
	fclose(fptr);
}

void incr_pid() {
	pid_count++;
}

// create logging for each action
// logging function should be its own file or kernel file (up to us)
// make function tab delimited
pcb* k_process_create(pcb* parent, ucontext_t * context) {
	// create new child thread and associated PCB
	// New thread should retain many properties of parent
	// returns a reference to a new PCB
	
	// pgid is set to -2 by default
	if (parent == NULL){
		// we in the shell G
		pcb* new_pcb = create_PCB(pid_count, -2, -2, context, FALSE, FALSE, 0);
		
		priority_queue_zero = add_PCB(priority_queue_zero, new_pcb);

		prev_cur = new_pcb;
		pid_count ++;

		return new_pcb;

	}else{
		pcb* new_pcb = create_PCB(pid_count, -2, parent->pid, context, parent->bool_read, parent->bool_write, 0);
		add_child_PCB(parent, new_pcb);
		priority_queue_zero = add_PCB(priority_queue_zero, new_pcb);
		// @Marty
		prev_cur = new_pcb;
		pid_count ++;

		return new_pcb;
	}


}

	// impl wise better to do null to avoid double malloc

// for every process check if parent is blocked. if it is then unblock!
void k_process_kill(pcb * process, int signal) {
	// use signal to kill the process referenced by process

	// My guess:
	// One big-ass if-statement to decide how the process in question should respond to the signal
	// Have to write some functions in pcb.c to decide how to modify pcb based on signal
	// Will probably need to dequeue the process and reenqueue for some cases, we will need a helper function for that
	// Will need to define all signals as constants at top of file

	// check parent
	pcb * daddy;
	int blocked_parent = FALSE;
	pcb * parent_queue = NULL;
	daddy = process -> parent_pcb;
	if (daddy != NULL){
		if (daddy->priority == 0){
			parent_queue = priority_queue_zero;
		} else if (daddy->priority == 1){
			parent_queue = priority_queue_one;

		} else{
			parent_queue= priority_queue_neg;
		}
	}
	// daddy = find_PCB_pid(priority_queue_zero, process->ppid);
	// if (daddy == NULL){
	// 	daddy = find_PCB_pid(priority_queue_neg, process->ppid);
	// 	if (daddy == NULL){
	// 		daddy = find_PCB_pid(priority_queue_one, process->ppid);
	// 		if (daddy != NULL){
	// 			parent_queue = priority_queue_one;
	// 		}
	// 	} else{
	// 		parent_queue = priority_queue_neg;
	// 	}
	// } else{
	// 	parent_queue = priority_queue_zero;
	// }
	if (daddy != NULL && daddy->status == BLOCKED){
		blocked_parent = TRUE;
	}
	//handle signals

	// instead of context switching directtly wait for scheduler to run again
	// check before you run that there is a process with a child changed flag turned on
	// that process becomes ur new curr_pcb
	// @Nick if no parent is blocked removed from queue al together

	if (signal == S_SIGSTOP){
		// log signaled
		fprintf(get_fptr(), "[%d]\t\tSIGNALED\t\t%d\t\t%d\n", get_timer_total(), process->pid, process->priority);
		if (process->status == READY){
			process->status = STOPPED;
			//log
			fprintf(fptr, "[%d]\t\tSTOPPED\t\t\t%d\t\t%d\t\t%s\n", timer_total, process->pid, process->priority, process->command);

			process->changed = TRUE;
			if (blocked_parent){
				daddy->status = READY;
				// log
				fprintf(fptr, "[%d]\t\tUNBLOCKED\t\t%d\t\t%d\t\t%s\n", timer_total, daddy->pid, daddy->priority, daddy->command);
				//remove parent and add to head of queueu
				if (parent_queue == priority_queue_neg){
					priority_queue_neg = remove_PCB(parent_queue, daddy);
					priority_queue_neg = add_to_head(priority_queue_neg, daddy);
				} else if(parent_queue == priority_queue_one){
					priority_queue_one = remove_PCB(parent_queue, daddy);
					priority_queue_one = add_to_head(priority_queue_one, daddy);

				} else if(parent_queue == priority_queue_zero){
					priority_queue_zero = remove_PCB(parent_queue, daddy);
					priority_queue_zero = add_to_head(priority_queue_zero, daddy);
				}
			}
		}
		
	} else if (signal == S_SIGCONT){
		// log signaled
		fprintf(get_fptr(), "[%d]\t\tSIGNALED\t\t%d\t\t%d\n", get_timer_total(), process->pid, process->priority);

		if (process->status == STOPPED){
			process->status = READY;
			// log
			fprintf(fptr, "[%d]\t\tCONTINUED\t\t%d\t\t%d\t\t%s\n", timer_total, process->pid, process->priority, process->command);
			process->changed = TRUE;
			if (blocked_parent){
				daddy->status = READY;
				// log
				fprintf(fptr, "[%d]\t\tUNBLOCKED\t\t%d\t\t%d\t\t%s\n", timer_total, daddy->pid, daddy->priority, daddy->command);
				if (parent_queue == priority_queue_neg){
					priority_queue_neg = remove_PCB(parent_queue, daddy);
					priority_queue_neg = add_to_head(priority_queue_neg, daddy);
				} else if(parent_queue == priority_queue_one){
					priority_queue_one = remove_PCB(parent_queue, daddy);
					priority_queue_one = add_to_head(priority_queue_one, daddy);

				} else if(parent_queue == priority_queue_zero){
					priority_queue_zero = remove_PCB(parent_queue, daddy);
					priority_queue_zero = add_to_head(priority_queue_zero, daddy);
				}
				
			}
		}
		
	} else if (signal == S_SIGTERM){
		if (process->status != ZOMBIE){
			if (process->terminated_natty == TRUE) {
				fprintf(get_fptr(), "[%d]\t\tEXITED\t\t\t%d\t\t%d\n", get_timer_total(), process->pid, process->priority);
				process->status = ZOMBIE;
				//printf("here!!\n");
				zombie_queue = add_PCB(zombie_queue, process);
			} else {
				fprintf(get_fptr(), "[%d]\t\tSIGNALED\t\t%d\t\t%d\n", get_timer_total(),process->pid, process->priority);
				process->status = KILLED;
			}
			fprintf(fptr, "[%d]\t\tZOMBIE\t\t\t%d\t\t%d\t\t%s\n", timer_total, process->pid, process->priority, process->command);
			//fprintf(fptr, "num_children[%d]\n", process->num_children);
			int num_c =process->num_children;
			for(int j = 0; j < num_c; j ++){
				// k process terminates updates the LIST!!!!!!! so ur checking an array thats is changing 
				fprintf(fptr, "[%d]\t\tORPHAN\t\t\t%d\t\t%d\t\t%s\n", timer_total, process->child_list[0]->pid, process->child_list[0]->priority, process->child_list[0]->command);
				k_process_terminate(process->child_list[0]);
			}
			process->num_children = 0;
			process->changed = TRUE;
			// init cleans up zombies in scenario of P, C,C
			// not blocked parent log zombie
			// create orpan log --> go through children.
			if (blocked_parent){
				daddy->status = READY;
				// log
				fprintf(fptr, "[%d]\t\tUNBLOCKED\t\t%d\t\t%d\t\t%s\n", timer_total, daddy->pid, daddy->priority, daddy->command);
				if (parent_queue == priority_queue_neg){
					priority_queue_neg = remove_PCB(parent_queue, daddy);
					priority_queue_neg = add_to_head(priority_queue_neg, daddy);
				} else if(parent_queue == priority_queue_one){
					priority_queue_one = remove_PCB(parent_queue, daddy);
					priority_queue_one = add_to_head(priority_queue_one, daddy);

				} else if(parent_queue == priority_queue_zero){
					priority_queue_zero = remove_PCB(parent_queue, daddy);
					priority_queue_zero = add_to_head(priority_queue_zero, daddy);
				}
			}
		}
		// log
	}
	if (daddy != NULL){
		add_child_PCB(daddy,process);
	}



	
}

// UNTESTED (Is this still true as of 11/21?)
void k_process_terminate(pcb * process) {
	// called when a thread returns or is terminated
	// perform any necessary clean up, such as freeing memory, setting the status of the child, etc
	// Handle children processes, kill em all
	// Kill parent (jeez)
	// Valgrind like a motherfucker
	// go through children and grand children etc and kill every single one of them aka remove from their respectove queues

	// after children removed, remove the process 


	// make sure removing everything from zombied and running
	if (process == NULL) return;

	for (int i = 0; i < process->num_children; i++) {
		pcb * child = process->child_list[i];
		
		k_process_terminate(child);
		fprintf(fptr, "[%d]\t\tORPHAN\t\t\t%d\t\t%d\t\t%s\n", timer_total, process->pid, process->priority, process->command);
		// log at exit time

	}

	// if status is ZOMBIE, add to zombie queue
	// if (process->status == ZOMBIE) {
	// 	add_PCB(zombie_queue, process);
	// }
	//add_PCB(zombie_queue, process);

	


	remove_child_PCB(process);

	// is this neccessary?
	zombie_queue = remove_PCB(zombie_queue,process);

	priority_queue_neg = remove_PCB(priority_queue_neg,process);
	priority_queue_zero = remove_PCB(priority_queue_zero, process);
	priority_queue_one = remove_PCB (priority_queue_one, process);
	free_PCB(process);


}



// checks every thread for process
pcb * k_get_process_pid(int p){
	pcb * process;
	process = find_PCB_pid(priority_queue_zero, p);
	if (process == NULL){
		process = find_PCB_pid(priority_queue_neg, p);
		if (process == NULL){
			process = find_PCB_pid(priority_queue_one, p);
			if (process == NULL && get_cur_pcb()->pid == p ){
				process = get_cur_pcb();
			}
		}
	}
	return process;

}


// get hard copy instead
pcb * k_get_current_process() {
	return cur_pcb;
}

void k_process_update_changed(pcb * process, int c){
	process->changed = c;
}

void k_process_nice(int p, int prior){
	pcb * change_pcb = k_get_process_pid(p);
	// check if curr pcb handle later
	// printf("inside k process nice: %d\n", p);
	if (change_pcb == NULL){
		return;
	}
	if (change_pcb == cur_pcb){
		change_pcb->priority = prior;
		// printf("inside k process nice: %d\n", p);
		fprintf(fptr, "[%d]\t\tNICE\t\t\t%d\t\t%d\t\t%s\n", timer_total, p, prior, change_pcb->command);
		return;
	}
	// nothing to change
	if (change_pcb->priority == prior){
		return;
	}
	// queue currently in
	if (change_pcb->priority == 1){
		priority_queue_one = remove_PCB(priority_queue_one, change_pcb);
	} else if (change_pcb->priority == 0){
		priority_queue_zero = remove_PCB(priority_queue_zero, change_pcb);
	} else if (change_pcb->priority == -1){
		priority_queue_neg = remove_PCB(priority_queue_neg, change_pcb);
	}
	// new queue
	if (prior == 1){
		change_pcb->priority = 1;
		priority_queue_one = add_PCB(priority_queue_one, change_pcb);
		if (change_pcb->parent_pcb != NULL){
			add_child_PCB(change_pcb->parent_pcb, change_pcb);
		}
	} else if (prior == 0){
		change_pcb->priority = 0;
		priority_queue_zero = add_PCB(priority_queue_zero, change_pcb);
		if (change_pcb->parent_pcb != NULL){
			add_child_PCB(change_pcb->parent_pcb, change_pcb);
		}
	} else if (prior == -1){
		change_pcb->priority = -1;
		priority_queue_neg = add_PCB(priority_queue_neg, change_pcb);
		if (change_pcb->parent_pcb != NULL){
			add_child_PCB(change_pcb->parent_pcb, change_pcb);
		}

	}
	// log nice (priority)
	fprintf(fptr, "[%d]\t\tNICE\t\t\t%d\t\t%d\t\t%s\n", timer_total, p, prior, change_pcb->command);
}

// rename function
void k_process_swap_to_scheduler(){
	// to be called when hanging wait
	// block current_pcb
	//  then swap context to scheduler (setcontext to scheduler)
	cur_pcb->status = BLOCKED;


	// log
	fprintf(fptr, "[%d]\t\tBLOCKED\t\t\t%d\t\t%d\t\t%s\n", timer_total, cur_pcb->pid, cur_pcb->priority, cur_pcb->command);

	if (cur_pcb->priority == -1){
		priority_queue_neg= add_PCB(priority_queue_neg,cur_pcb);
	} else if (cur_pcb->priority == 1){
		priority_queue_one = add_PCB(priority_queue_one,cur_pcb);
	} else{
		priority_queue_zero = add_PCB(priority_queue_zero,cur_pcb);
	}
	// jump to scheduler
	swapcontext(cur_pcb->context, scheduler_context);
}

void mkcontext(ucontext_t *uc,  void *function, int argc, char const *argv[]) {
	// initialize "uc" to the current context (i.e. getcontext)
	// initialize the ucontext structure, give it a stack, flags, and sigmask
	// make the context with the appropriate function
	// create a context for scheduler and set uc_link to this context

	getcontext(uc); // set uc to current context

	void * stack = malloc(STACKSIZE); // create stack for uc
    if (stack == NULL) {
        perror("malloc");
        exit(1);
    }

    //VALGRIND_STACK_REGISTER(stack, stack + STACKSIZE);

    uc->uc_stack.ss_sp = stack; // give stack to uc
    uc->uc_stack.ss_size = STACKSIZE; // uc size
    uc->uc_stack.ss_flags = 0; // flags is bullshit
    
    // errors with the signal mask
    if (sigemptyset(&uc->uc_sigmask) < 0){
    	perror("sigemptyset");
    	free(stack);
    	exit(1);
    }
    
    uc->uc_link = scheduler_context;

    if (argv == NULL) {
    	makecontext(uc, function, 1);
    	return;
    }

    int num_args = argc;
    if (num_args == 2){
    	makecontext(uc, function, 2, argv[1]);
    } else if (num_args == 3){
    	makecontext(uc, function, 3, argv[1], argv[2]);

    } else if (num_args == 4){
    	makecontext(uc, function, 4, argv[1], argv[2], argv[3]);

    } else{
    	makecontext(uc, function, 1); // create the context with the appropriate function
	}
}

// we need to keep track of this to free it
info * create_info(int pid, int status, char * command,
		int priority, FD_LIST* fd_list, int ground) {
	info * new_info = malloc(sizeof(info));
	new_info->pid = pid;
	new_info->status = status;
	new_info->command = command;
	new_info->priority = priority;
	new_info->fd_list = fd_list;
	new_info->ground = ground;
	return new_info;
}

void free_info(info * info)
{
	free(info);
}

info * k_info(int pid) {
	pcb * result = find_PCB_pid(priority_queue_neg, pid);
	if (result == NULL) find_PCB_pid(priority_queue_zero, pid);
	if (result == NULL) find_PCB_pid(priority_queue_one, pid);
	if (result == NULL) return NULL;

	return create_info(
		result->pid,
		result->status,
		result->command,
		result->priority,
		result->fd_list,
		result->ground
	);
}

info * k_get_current_info()
{
	pcb * current = get_prev_pcb();
	return k_info(current -> pid);
}

void k_end_everything() {
	end_everything = TRUE;
	swapcontext(get_prev_pcb()->context, get_scheduler_context());
}

// Sets up text file for logging
void setup_file() {
	set_fptr(fopen("log/logger.txt", "w"));

	if (get_fptr() == NULL) {
		perror("null file");
	}
}

void setup_timer() {
	it.it_interval.tv_sec = 0;
    it.it_interval.tv_usec = INTERVAL * 1000;
    it.it_value = it.it_interval;
    if (setitimer(ITIMER_REAL, &it, NULL) ) perror("setitiimer");
}

int get_timer_mod() {
	return timer_mod;
}

void incr_timer_mod() {
	timer_mod++;
}

void mod_timer_mod() {
	timer_mod %= 19;
}

sigset_t * get_signal_mask() {
	return &signal_mask;
}

void * get_signal_stack() {
	return signal_stack;
}

void set_signal_stack() {
	signal_stack = malloc(STACKSIZE);
	VALGRIND_STACK_REGISTER(signal_stack, signal_stack + STACKSIZE);
}

void timeout() {
	how_finished = TIMEOUT;
}

void normal_finish() {
	how_finished = NORMAL_FINISH;
}

int get_how_finished() {
	return how_finished;
}

void idle() {
	// idle context
	// printf("idle entered\n");
	sigsuspend(get_signal_mask());
}

void setup_idle() {
	idle_context = malloc(sizeof(ucontext_t));
}

void reset_idle() {
	getcontext(idle_context);
    idle_context->uc_stack.ss_sp = get_signal_stack();
    idle_context->uc_stack.ss_size = STACKSIZE;
    idle_context->uc_stack.ss_flags = 0;
    sigemptyset(&(idle_context->uc_sigmask));
    makecontext(idle_context, idle, 1);
}

ucontext_t * get_idle() {
	return idle_context;
}

void scheduler() {
	/*
	if (get_zero() == NULL) {
		printf("zero is null\n");
	}
	else {
		printf("------------\n");
		pcb * temp8 = get_zero();
		while (temp8 != NULL) {
			printf("%d    %d\n", temp8->pid, temp8->status);
			temp8 = temp8->next;
		}
		printf("------------\n");
	}
	*/
	//printf("just entered scheduler\n");
	// NOTE: run "top -d 1.9 | grep PennOS" to get accurate breakdowns

	// if (dummy_exit == TRUE){
	// 	fclose_fptr();
	// 	return;
	// }

	// scheduler should end
	if (end_everything == TRUE){
		// clean up
		//free scheduler stack

		//exited log
		fprintf(get_fptr(), "[%d]\t\tEXITED\t\t\t%d\t\t%d\n", get_timer_total(), get_prev_pcb()->pid, get_prev_pcb()->priority);

		/*
		pcb * cur_pos = priority_queue_neg;
		while(cur_pos != NULL){
			cur_pos = remove_PCB(cur_pos,cur_pos);
			free(cur_pos);
		}
		cur_pos = priority_queue_one;
		while(cur_pos != NULL){
			cur_pos = remove_PCB(cur_pos,cur_pos);
			free(cur_pos);
		}
		cur_pos = priority_queue_zero;
		while(cur_pos != NULL){
			cur_pos = remove_PCB(cur_pos,cur_pos);
			free(cur_pos);
		}
		*/
		VALGRIND_STACK_DEREGISTER(scheduler_stack);
		VALGRIND_STACK_DEREGISTER(signal_stack);
		free(scheduler_context);
		free(idle_context);

		fclose_fptr();
		return;
	}
	if (get_how_finished() == TIMEOUT) {
		// switch queues
		if (get_timer_mod() <= 4) {
			current_queue = 1;
		} else if (get_timer_mod() <= 10) {
			current_queue = 0;
		} else if (get_timer_mod() <= 19) {
			current_queue = -1;
		}

		mod_timer_mod();
	} else {
		// shell takes in IO and comes straight here insead of TIMEOUT?
		if (get_cur_pcb() != NULL && get_cur_pcb()->parent_pcb != NULL && get_cur_pcb()->pid > 0 && get_cur_pcb()->parent_pcb->pid >= 0 && get_cur_pcb()->status != ZOMBIE && get_cur_pcb()->status != KILLED && get_cur_pcb()->signal_flag == 0) {
			// terminates gracefully or hits sigint
			// process terminated naturally, update the flag 

			get_cur_pcb()->terminated_natty = 1;

			
			// printf("PID OF ZOMBIE%d\n", get_cur_pcb()->pid );
			// printf("PPID OF ZOMBIE%d\n", get_cur_pcb()->parent_pcb->pid );
			//fprintf(get_fptr(), "We here\n");
			//fprintf(get_fptr(), "pid : %d    ;    cur pcb num children:  %d\n", get_cur_pcb()->pid, get_cur_pcb()->num_children);
			k_process_kill(get_cur_pcb(), S_SIGTERM);
			//fprintf(get_fptr(), "pid : %d    ;    cur pcb num children. 2:  %d\n", get_cur_pcb()->pid, get_cur_pcb()->num_children);

		}
	}

	set_cur_pcb(NULL);

	normal_finish();

	// THIS SETS SCHEDULER TO BE READY EVEN THOUGH ITD FUCKING BLOCKED
	// checks through queues for sleeping processes
	pcb * checker = get_neg();
	while (checker != NULL) {
		if (checker->sleep_time > 0) {
			checker->sleep_time--;
		}
		else if (checker->sleep_time == 0) {
			checker->status = READY;
			checker->sleep_time = -1;
		}
		checker = checker->next;
	}
	checker = get_zero();
	while (checker != NULL) {
		if (checker->sleep_time > 0) {
			checker->sleep_time--;
		}
		else if (checker->sleep_time == 0)  {
			checker->status = READY;
			checker->parent_pcb->status = READY;
			checker->sleep_time = -1;
		}
		checker = checker->next;
	}
	checker = get_one();
	while (checker != NULL) {
		if (checker->sleep_time > 0)  {
			checker->sleep_time--;
		}
		else if (checker->sleep_time == 0)  {
			checker->status = READY;
			checker->sleep_time = -1;
		}
		checker = checker->next;
	}
	checker = NULL;

	reset_idle();
    
	pcb* candidate;

	if (current_queue == -1) {
		candidate = find_next_valid(get_neg());
		if (candidate == NULL) {
			// switch to idle
			//printf("idle\n");
			setcontext(get_idle());
		} else {
			//printf("not idle\n");
			// enqueue process
			pcb * rem = remove_PCB(get_neg(), candidate);
			set_neg(rem);
			//printf("candidate->next:  %d\n", candidate->next);
			if (candidate->parent_pcb != NULL){
				add_child_PCB(candidate->parent_pcb, candidate);
			}
		}
	} else if (current_queue == 0) {
		candidate = find_next_valid(get_zero());
		if (candidate == NULL) {
			// switch to idle
			//printf("idle\n");
			setcontext(get_idle());
		} else {
			//printf("not idle\n");
			// enqueue process
			//printf("before get zero\n");
			set_zero(remove_PCB(get_zero(), candidate));
			//printf("after get zero\n");
			//printf("candidate->next:  %d\n", candidate->next);
			if (candidate->parent_pcb != NULL){
				add_child_PCB(candidate->parent_pcb , candidate);
			}
		}
	} else {
		candidate = find_next_valid(get_one());
		if (candidate == NULL) {
			// switch to idle
			//printf("idle\n");
			setcontext(get_idle());
		} else {
			//printf("not idle\n");
			// enqueue process
			set_one(remove_PCB(get_one(), candidate));
			//printf("candidate->next:  %d\n", candidate->next);
			if (candidate->parent_pcb != NULL){
				add_child_PCB(candidate->parent_pcb , candidate);
			}
		}
	}


	// set prev_fg and prev_cur
	// for descriptions of these variables, refer to the top of kernel_functions.c
	if (candidate != NULL && candidate->ground == FG) {
		prev_fg = candidate;
	}
	if (candidate != NULL) {
		prev_cur = candidate;
		//printf("prev pid:  %d\n", prev_fg->pid);
		if (candidate->pid == 1) {
			// printf("running shell process\n");
		}		
		if (candidate->pid == 2) {
			//printf("running background process\n");
		}
		if (candidate->pid == 3) {
			//printf("running busy 1 process\n");
		}
		if (candidate->pid == 4) {
			//printf("running busy 2 process\n");
		}
		else {
			//printf("idle\n");
		}

	}

	set_cur_pcb(candidate);


	// timer_total, SCHEDULE, cur_pcb->pid, current_queue, *PROCESS_NAME*
	fprintf(get_fptr(), "[%d]\t\t%s\t\t%d\t\t%d\n", get_timer_total(), "SCHEDULE", get_cur_pcb()->pid, get_cur_pcb()->priority);
	// context switch
	setcontext(get_cur_pcb()->context);
}

pcb * get_queue(int queue) {
	if (queue == -1) {
		return get_neg();
	} else if (queue == 0) {
		return get_zero();
	} else if (queue == 1) {
		return get_one();
	} else {
		printf("invalid queue\n");
		return NULL;
	}
}

void timer_interrupt() {
	// Create new scheduler context
	//printf("interrupt called\n");
    getcontext(get_scheduler_context());
    reset_scheduler();

    timeout();

    incr_timer();
    incr_timer_mod();

    // JONAH U GOD
    if (get_cur_pcb() != NULL) {
    	// reenqueues current process
    	if (get_cur_pcb()->priority == -1) {
	    	set_neg(add_PCB(get_neg(), get_cur_pcb()));
	    } else if (get_cur_pcb()->priority == 0) {
	    	set_zero(add_PCB(get_zero(), get_cur_pcb()));
	    } else if (get_cur_pcb()->priority == 1) {
	    	set_one(add_PCB(get_one(), get_cur_pcb()));
	    }
	    swapcontext(get_cur_pcb()->context, get_scheduler_context());
    } else {
    	// if no unfinished process just switch to scheduler
    	setcontext(get_scheduler_context());
    }
}


void setup_signals() {
    struct sigaction act;

    act.sa_sigaction = (void *) timer_interrupt; 
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_RESTART | SA_SIGINFO;

    sigemptyset(get_signal_mask());

    if (sigaction(SIGALRM, &act, NULL) != 0) {
        perror("Signal handler");
    }
}
#ifndef _KERNEL_FUNCTIONS_H
#define _KERNEL_FUNCTIONS_H

#include <ucontext.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include "pcb.h"
#include <valgrind/valgrind.h>

#define S_SIGSTOP 0 // SIGSTOP
#define S_SIGCONT 1 // SIGCONT
#define S_SIGTERM 2 // SIGTERM
#define STACKSIZE (SIGSTKSZ *2)              /* stack size */
#define INTERVAL 100                /* timer interval in nanoseconds */
#define TIMEOUT 1
#define NORMAL_FINISH 0

typedef struct p_info_struct {
	int pid;
	int status;
	char * command;
	int priority;
	FD_LIST* fd_list;
	int ground;
} info;

void set_shell_pcb(pcb * p);

pcb * get_shell_pcb();

pcb * get_prev_pcb();

pcb * get_prev_fg_pcb();

void set_prev_pcb(pcb * new_prev);

void set_prev_fg_pcb(pcb * new_prev);

pcb * get_neg();

void set_neg(pcb * neg);

pcb * get_zero();

void set_zero(pcb * zero);

pcb * get_one();

void set_one(pcb * one);

pcb * get_zombies();

pcb * get_cur_pcb();

void set_cur_pcb(pcb * new_pcb);

ucontext_t * get_scheduler_context();

void reset_scheduler();

void setup_scheduler();

int get_timer_total();

void incr_timer();

FILE * get_fptr();

void set_fptr(FILE * f);

void fclose_fptr();

pcb* k_process_create(pcb* parent, ucontext_t * context);

void incr_pid();

void k_process_kill(pcb * process, int signal);

void k_process_terminate(pcb * process);

pcb * k_get_current_process();

void mkcontext(ucontext_t *uc,  void *function, int argc, char const *argv[]);

void k_process_swap_to_scheduler();

pcb * k_get_process_pid(int pid);

void k_process_update_changed(pcb * process, int c);

void k_process_nice(int p, int prior);

info * create_info(int pid, int status, char * command,
		int priority, FD_LIST* fd_list, int ground);

void free_info(info * info);

info * k_info(int pid);

info * k_get_current_info();

void k_end_everything();

void setup_file();

void setup_timer();

int get_timer_mod();

void incr_timer_mod();

void mod_timer_mod();

sigset_t * get_signal_mask();

void * get_signal_stack();

void set_signal_stack();

void timeout();

void normal_finish();

int get_how_finished();

void idle();

void setup_idle();

void reset_idle();

ucontext_t * get_idle();

void scheduler();

pcb * get_queue(int queue);

void timer_interrupt();

void setup_signals();

#endif
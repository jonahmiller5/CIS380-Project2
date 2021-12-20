#ifndef _USER_FUNCTIONS_H
#define _USER_FUNCTIONS_H

#include <ucontext.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <poll.h>
#include "kernel_functions.h"
#include "fs/descriptors.h"

#define NOHANG 0
#define HANG 1

// create externs for wif process statuses
// every p_ function set them to FALSE

typedef struct p_wait_struct {
	int pid;
	int status;
} p_wait_struct;


int W_WIFEXITED(int status);

int W_WIFSTOPPED(int status);

int W_WIFCONTINUED(int status);

int W_WIFSIGNALED(int status);

p_wait_struct * create_p_wait_struct(int pid, int status);

int p_spawn(void * func, int argc, char const *argv[], int ground);

void p_kill(int pid, int signal);

p_wait_struct * p_wait(int mode, int given_pid);

void p_exit();

void p_nice(int pid, int priority);

// void dummy1();

// void dummy2();

// void dummy3();

// void dummy4();

void empty();

void _sleep(char * t_str);

void p_sleep(int ticks);

info * p_info(int pid);

void zombie_child();

void zombify();

void orphan_child();

void orphanify();

void p_end_everything();

int p_pid_present(int pid);

void p_nice_pid();

#endif
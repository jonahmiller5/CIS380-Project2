#ifndef _SHELL_H
#define _SHELL_H

#include <string.h>
#include <unistd.h>
#include <stdlib.h> 
#include <stdio.h>
#include <signal.h>
#include "user_functions.h"
#include "tokenizer.h"
#include "jobs.h"
#include "fs/filesystem.h"

#define TRUE 1
#define FALSE 0
#define STR_BYTYES 1024
#define EXIT_SUCCESS 0

#define SHELL_PROMPT "penn-os>  "

typedef struct history_struct {
	char * cmd;
	struct history_struct * next;
} hist_node;

hist_node * create_hist_node(char * cmd);

hist_node * add_hist(hist_node * head, hist_node * new_node);

void signal_handler(int signo);

int has_ampersand(char* user_input);

char check_input(char * user_input);

void busy();

void man();

void ps();

void history();

void shell_nice(char * priority, char * command, char * optional_arg);

void touch(char* filename);

void rm(char* filename);

int shell();

void run_os();

#endif
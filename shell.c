#include "shell.h"
#include "fs/filesystem.h"

int pid;
struct Job * head;

hist_node * cmds;

int INPUT_SEEN = FALSE;
int OUTPUT_SEEN = FALSE;
int APPEND_SEEN = FALSE;

hist_node * create_hist_node(char * text) {
	if (text == NULL) return NULL;

	hist_node * new_node = calloc(1,sizeof(hist_node));
	new_node->cmd = text;
	new_node->next = NULL;

	return new_node;
}

hist_node * add_hist(hist_node * head, hist_node * new_node) {
	if (new_node == NULL) return head;
	if (head == NULL) return new_node;

	hist_node * traveler = head;
	while (traveler->next != NULL) {
		traveler = traveler->next;
	}

	traveler->next = new_node;

	return head;
}

void signal_handler(int signo) {
	if (signo == SIGINT && pid != 1){
		get_prev_fg_pcb()->signal_flag = 1;
		p_kill(get_prev_fg_pcb()->pid, S_SIGTERM);

		// pcb * temp = get_prev_fg_pcb();
		// if (temp == get_shell_pcb()) {
		// 	pcb * actual;
		// 	actual = find_PCB_pid(get_neg(), pid);
		// 	if (actual == NULL){
		// 		actual = find_PCB_pid(get_one(), pid);
		// 		if (actual == NULL){
		// 			actual = find_PCB_pid(get_zero(),pid);
		// 		}
		// 	}
		// 	if (actual != NULL){
		// 		set_prev_fg_pcb(actual);
		// 	}
		// }
		write(STDOUT_FILENO, "\n", 1);
		//p_kill(pid, S_SIGTERM);
		setcontext(get_scheduler_context());
	}
	if (signo == SIGINT && pid == 1){
		write(STDOUT_FILENO, "\n", 1);
		// get_prev_fg_pcb()->status = READY;
		if (get_prev_fg_pcb()->status == BLOCKED) {
			get_prev_fg_pcb()->status = READY;
			setcontext(get_scheduler_context());
		}
		write(STDOUT_FILENO, "penn-os>  ", 10);
		setcontext(get_prev_fg_pcb()->context);
	}
	if (signo == SIGTSTP && pid != 1){
		get_prev_fg_pcb()->signal_flag = 1;

		// pcb * temp = get_prev_fg_pcb();
		// if (temp == get_shell_pcb()) {
		// 	pcb * actual;
		// 	actual = find_PCB_pid(get_neg(), pid);
		// 	if (actual == NULL){
		// 		actual = find_PCB_pid(get_one(), pid);
		// 		if (actual == NULL){
		// 			actual = find_PCB_pid(get_zero(),pid);
		// 		}
		// 	}
		// 	if (actual != NULL){
		// 		set_prev_fg_pcb(actual);
		// 	}
		// }
		printf("pid of stopped:  %d\n", get_prev_fg_pcb()->pid);
		p_kill(get_prev_fg_pcb()->pid, S_SIGSTOP);

		// if cur pcb has been removed
		if (get_cur_pcb() != NULL && get_cur_pcb()->pid == get_prev_fg_pcb()->pid){
			pcb * s_pcb = get_prev_fg_pcb();
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

		}
		setcontext(get_scheduler_context());
		//setcontext(get_shell_pcb()->context);
	}
	if (signo == SIGTSTP && pid == 1) {
		if (get_prev_fg_pcb()->status != BLOCKED) {
			write(STDOUT_FILENO, "\npenn-os>  ", 11);
			setcontext(get_prev_fg_pcb()->context);
		}
		setcontext(get_scheduler_context());
	}
	

}


// man functions lists all available commands
void man() {
	f_write(F_STDOUT, "cat\n", 4);
	f_write(F_STDOUT, "nice\n", 5);
	f_write(F_STDOUT, "sleep\n", 6);
	f_write(F_STDOUT, "busy\n", 5);
	f_write(F_STDOUT, "ls\n", 3);
	f_write(F_STDOUT, "touch\n", 6);
	f_write(F_STDOUT, "rm\n", 3);
	f_write(F_STDOUT, "ps\n", 3);
	f_write(F_STDOUT, "zombify\n", 8);
	f_write(F_STDOUT, "orphanify\n", 10);
	f_write(F_STDOUT, "nice_pid\n", 9);
	f_write(F_STDOUT, "man\n", 4);
	f_write(F_STDOUT, "bg\n", 3);
	f_write(F_STDOUT, "fg\n", 3);
	f_write(F_STDOUT, "jobs\n", 5);
	f_write(F_STDOUT, "logout\n", 7);
	// 3 custom functions
	f_write(F_STDOUT, "history\n", 9);
	f_write(F_STDOUT, "reset\n", 6);
	f_write(F_STDOUT, "rand_chars\n", 11);

}

void dummy7(char * arg1) {
	printf("--------\n");
	printf("dummy7\n");
	printf("our arg: %s\n", arg1);
	printf("dummy7\n");
	printf("--------\n");
}

// Hard coded for now
void shell_nice(char* priority, char * command, char * optional_arg) {
	printf("test&\n");
	printf("command 1: %s\n", priority);
	printf("command 2: %s\n", command);
	printf("command 3: %s\n", optional_arg);
	//printf("command 4: %s\n", argv[1]);
	
	// printf("inside shell nice\n");
	// create new argv for dummy7 {dummy7, }
	char * yinger[2] = {"dummy7", optional_arg};
	//dummy 7
	pid = p_spawn(dummy7, 2, yinger, 0);
	// printf("inside shell nice after p_spawn: %d\n", pid);
	p_nice(pid, priority);
}

/*
Helper function checking if ampersand is in user input.
*/

int has_ampersand(char* user_input) {
	for (int i = strlen(user_input) - 2; i >= 0; i--) {
		if (user_input[i] == ' ') {
			continue;
		}

		if (user_input[i] == '&') {
			return TRUE;
		} else {
			return FALSE;
		}
	}
	return FALSE;
}

/**
*  Helper function to check if user input is formatted correctly.
*/
char check_input(char * user_input) {
	// first char is not a pipe or redirect or ampersand
	if (user_input[0] == '<') {
		return '<';
	}

	if (user_input[0] == '>') {
		return '>';
	}

	if (user_input[0] == '&' ) {
		return '&';
	}

	// last char is not a pipe or redirect
	if (user_input[strlen(user_input) - 2] == '<' || 
		user_input[strlen(user_input) - 2] == '>') {
		return '\n';
	}

	// make sure there are no syntax issues between redirects and pipes
	int num_input = 0;
	int num_output = 0;
	int num_amper = 0;

	char last_seen = '\0';
	for (int i = 0; i < strlen(user_input); i++) {
		if (user_input[i] == '|') return '|';

		if (user_input[i] == '&') {
			if (last_seen == '<' || last_seen == '>' || last_seen == '&') {
				return '&';
			} else if (num_amper > 0) {
				return '&';
			}

			num_amper++;
		} else if (user_input[i] == '<') {
			INPUT_SEEN = TRUE;
			if (last_seen == '<' || last_seen == '>' || last_seen == '&') {
				return '<';
			} else if (num_input > 0) {
				return '<';
			}

			num_input++;
		} else if (user_input[i] == '>') {
			if (APPEND_SEEN == TRUE) return '>';
			if (num_output == 0) OUTPUT_SEEN = TRUE;
			if (last_seen == '<' || last_seen == '&') {
				return '>';
			}
			if (OUTPUT_SEEN && num_output > 0) {
				if (last_seen == '>') {
					OUTPUT_SEEN = FALSE;
					APPEND_SEEN = TRUE;
				} else {
					return '>';
				}
			}

			num_output++;
		}

		last_seen = user_input[i];
	}

	if (num_amper > 0) {
		int amper_found = has_ampersand(user_input);
		if (!amper_found) {
			return '&';
		}
	}

	return '\0';
}

void busy() {
	while (TRUE) {
	}
}

// to be replaced by sleep in bg
void busy_bg() {
	int num = 0;
	while (num < 100) {
		num ++;
	}
}

// UNTESTED
void ps() {
	//get_zero()->status=ZOMBIE;
	printf("PID\t\tPRIORITY\tSTATUS\t\t\tCMD\n");

	pcb * traveler = get_neg();
	while (traveler != NULL) {
		printf("%d\t\t%d\t\t", traveler->pid, traveler->priority);
		switch(traveler->status) {
			case RUNNING:
				printf("RUNNING \t\t%s\n", traveler->command);
				break;
			case BLOCKED:
				printf("BLOCKED \t\t%s\n", traveler->command);
				break;
			case FINISHED:
				printf("FINISHED\t\t%s\n", traveler->command);
				break;
			case READY:
				printf("READY   \t\t%s\n", traveler->command);
				break;
			case ZOMBIE:
				printf("ZOMBIE  \t\t%s\n", traveler->command);
				break;
			case ORPHAN:
				printf("ORPHAN  \t\t%s\n", traveler->command);
				break;
			case KILLED:
				printf("KILLED  \t\t%s\n", traveler->command);
				break;
			case STOPPED:
				printf("STOPPED \t\t%s\n", traveler->command);
				break;
		}
		traveler = traveler->next;
	}
	traveler = get_zero();
	while (traveler != NULL) {
		printf("%d\t\t%d\t\t", traveler->pid, traveler->priority);
		switch(traveler->status) {
			case RUNNING:
				printf("RUNNING \t\t%s\n", traveler->command);
				break;
			case BLOCKED:
				printf("BLOCKED \t\t%s\n", traveler->command);
				break;
			case FINISHED:
				printf("FINISHED\t\t%s\n", traveler->command);
				break;
			case READY:
				printf("READY   \t\t%s\n", traveler->command);
				break;
			case ZOMBIE:
				printf("ZOMBIE  \t\t%s\n", traveler->command);
				break;
			case ORPHAN:
				printf("ORPHAN  \t\t%s\n", traveler->command);
				break;
			case KILLED:
				printf("KILLED  \t\t%s\n", traveler->command);
				break;
			case STOPPED:
				printf("STOPPED \t\t%s\n", traveler->command);
				break;
		}
		traveler = traveler->next;
	}
	traveler = get_one();
	while (traveler != NULL) {
		printf("%d\t\t%d\t\t", traveler->pid, traveler->priority);
		switch(traveler->status) {
			case RUNNING:
				printf("RUNNING \t\t%s\n", traveler->command);
				break;
			case BLOCKED:
				printf("BLOCKED \t\t%s\n", traveler->command);
				break;
			case FINISHED:
				printf("FINISHED\t\t%s\n", traveler->command);
				break;
			case READY:
				printf("READY   \t\t%s\n", traveler->command);
				break;
			case ZOMBIE:
				printf("ZOMBIE  \t\t%s\n", traveler->command);
				break;
			case ORPHAN:
				printf("ORPHAN  \t\t%s\n", traveler->command);
				break;
			case KILLED:
				printf("KILLED  \t\t%s\n", traveler->command);
				break;
			case STOPPED:
				printf("STOPPED \t\t%s\n", traveler->command);
				break;
		}
		traveler = traveler->next;
	}
	//printf("before zombies\n");
	traveler = get_zombies();
	while (traveler != NULL) {
		printf("%d\t\t%d\t\tZOMBIE  \t\t%s\n", traveler->pid, traveler->priority, traveler->command);
		
		traveler = traveler->next;
	}	

	// print the currently running process!
	printf("%d\t\t%d\t\tRUNNING \t\t%s\n", get_prev_fg_pcb()->pid, get_prev_fg_pcb()->priority, get_prev_fg_pcb()->command);
}

void history() {
	hist_node * traveler = cmds;

	f_write(F_STDOUT, "\n", 1);

	while (traveler != NULL) {
		//printf("%s", traveler->cmd);
		f_write(F_STDOUT, traveler->cmd, strlen(traveler->cmd));
		traveler = traveler->next;
	}

	f_write(F_STDOUT, "\n", 1);
}



int shell() {
	int bytes;
	int status;
	// int terminal_control = 0;
	char redirect_failure[45] = "Invalid: syntax error near unexpected token `";
	char user_input[1024];
	char user_input_cpy[1024];
	TOKENIZER *jobs_or_other;
	char* tok;
	int amper_found = 0;
	struct Job * current_job = NULL;
	head = NULL;
	int job_counter = 0;
	// assuming its one of our dummy functions	signal(SIGINT, signal_handler);
	signal(SIGINT, signal_handler);
	signal(SIGTSTP, signal_handler);

	while(TRUE){
		pid = 1;
		memset(user_input, '\0',  1024 * sizeof(char));
		memset(user_input_cpy, '\0',  1024 * sizeof(char));
		if (head == NULL){
			job_counter = 0;
		}

		// wait on BG jobs

		struct Job * job_count = head;
		while (job_count != NULL) {
			p_wait_struct * wait_struct = p_wait(NOHANG, job_count->pid);
			if (wait_struct == NULL){
				// no imediate bg with that pid to wait on
				struct Job * next_job = job_count->next;
				job_count = next_job;
				continue;
			}
			status = wait_struct->status;
			// update statuses to see if 
			if (W_WIFSTOPPED(status)) {
				// make the job BG, set status stopped
				// essentially updates our jobs queue
				job_count->status = J_STOPPED;
				// checks what stuff is the latest modified
				job_count->last_modified_counter = job_counter;
				job_counter++;
				printf("Stopped: %s\n", current_job->user_input);
			}
				
			// unsure if needed
			if (W_WIFCONTINUED(status)){
				job_count->status = J_RUNNING;
				// checks what stuff is the latest modified
				job_count->last_modified_counter = job_counter;
				job_counter++;
				printf("Restarting: %s\n", current_job->user_input);
			}
			
			//if job finished
			if (W_WIFEXITED(status) || W_WIFSIGNALED(status)){
				job_count->status = J_DONE;
				// checks what stuff is the latest modified
				job_count->last_modified_counter = job_counter;
				job_counter++;
			}
			struct Job * next_job = job_count->next;
			job_count = next_job;
		}
		job_count = head;
		// clean up finished BG processes
		while (job_count != NULL) {
			if (job_count->status == J_STOPPED || job_count->bool_type == FG){
				job_count = job_count->next;
				continue;
			}
			if (job_count->status == J_DONE){
				char * command_tok;
				char command_cpy[1024];
				memset(command_cpy, '\0',  1024 * sizeof(char));
				strcpy(command_cpy, job_count->user_input);
				//printf("command copy:%s\n", command_cpy);
				command_tok = strtok(command_cpy,"&");
				// make sure to trim last element in 
				// printf("Finished--->for some reason & set to done: %s\n", command_tok);
				struct Job * new_job = job_count->next;
				head = remove_job_index(head, job_count->current_job_number);
				job_count = new_job;
				continue;
			}
			struct Job * next_job = job_count->next;
			job_count = next_job;
		}

		//handle input

		//write(STDOUT_FILENO, shell_name, 15);
		f_dup_reset();
		if (f_write(F_STDOUT, SHELL_PROMPT, strlen(SHELL_PROMPT)) == F_FAILED) {
			p_perror("shell prompt");
			break;
		}

		bytes = f_read(F_STDIN, user_input, 1024);

		if (bytes == F_FAILED){
			p_perror("Read Problem");
			break;
		}
		strcpy(user_input_cpy, user_input);
		// handle logout command
		if (strcmp(user_input, "logout\n") == 0){
			bytes = 0;
		}
		// handle exits (logout and ctrl d)
		if (bytes == 0 ){
			// if (head != NULL) {	//disabled this because of core dumped error
			// 	int i = 0;
			// 	struct Job * cur_job = head;
			// 	//cleans up job queueu
			// 	while (cur_job != NULL) {
			// 		// print_job(cur_job);
			// 		struct Job* removal = cur_job;
			// 		cur_job = cur_job->next;
			// 		free_job(removal);
			// 	}
			// }
			// flip flag to clean everything in scheduler
			//change to p function for abstraction
			printf("\n");
			p_end_everything();
			return(1);
			//break;
		}
		//handle 1024
		if (bytes >= 1024 && user_input[1023] != '\n') {
			user_input[1023] = '\0';

			char* clear_buf = calloc(1024, sizeof(char));
			int read_clear = read(STDIN_FILENO, clear_buf, 1024);
			while (read_clear > 1024) {
				read_clear = read(STDIN_FILENO, clear_buf, 1024);
			}

			int length = strlen(user_input);

			for (int i = 1024; i < length; i++) {
				user_input[i] = '\0';
			}
		}

		//press just enter
		if (user_input[0] == '\n'){
			continue;
		}

		INPUT_SEEN = FALSE;
		OUTPUT_SEEN = FALSE;
		APPEND_SEEN = FALSE;
		
		// check whether input has proper syntax
		//@JONAH double check
		char * hist_str = calloc(1024, sizeof(char));
		strcpy(hist_str, user_input);

		hist_node * cmd = create_hist_node(hist_str);
		cmds = add_hist(cmds, cmd);

		char valid_input = check_input(user_input);
		amper_found = has_ampersand(user_input);
		char help[2] = {valid_input, '\0'};
		if (valid_input != '\0') {
			f_write(F_STDOUT, "Invalid Input\n", 14);
			continue;
		}

		jobs_or_other = init_tokenizer(user_input);
		char* j_tok = get_next_token(jobs_or_other);
		if (j_tok == NULL) {
			continue;
		}

		// handle jobs
		if (strcmp(j_tok, "jobs") == 0) {
			free(j_tok);
			j_tok = get_next_token(jobs_or_other);
			if (j_tok == NULL) {
				struct Job * curr = head;
				while (curr != NULL) {
					char temp[5];
					memset(temp, '\0', sizeof(*temp));
					// figure this out
					if (curr->status == J_RUNNING) {
						// char * number = "";
						// itoa(curr->current_job_number,number, );
						// f_write(F_STDOUT, itoa(), strlen(SHELL_PROMPT));
						printf("[%d]   %s   (running)\n", curr->current_job_number, curr->user_input);
					}
					else if (curr->status == J_STOPPED) {
						printf("[%d]   %s   (stopped)\n", curr->current_job_number, curr->user_input);
					}
					curr = curr -> next;
				}
				free(j_tok);
				free_tokenizer(jobs_or_other);
				continue;
			} // else wit perror
		}
		// handle man
		if (strcmp(j_tok, "man") == 0) {
			free(j_tok);
			j_tok = get_next_token(jobs_or_other);
			if (j_tok == NULL) {
				man();
				free(j_tok);
				free_tokenizer(jobs_or_other);
				continue;
			} // else with perrror
		}
		// handle nice_pid
		if (strcmp(j_tok, "nice_pid") == 0) {
			free(j_tok);
			j_tok = get_next_token(jobs_or_other);
			if (j_tok != NULL) {
				int pr = atoi(j_tok);
				if (pr == 0 && !strcmp(pr,"0")){
					f_write(F_STDOUT, "Invalid Input\n", 14);
					continue;
				}
				free(j_tok);
				j_tok = get_next_token(jobs_or_other);
				if (j_tok != NULL){
					int p = atoi(j_tok);
					if (p == 0 && !strcmp(p,"0")){
						f_write(F_STDOUT, "Invalid Input\n", 14);
						continue;
					}
					free(j_tok);
					j_tok = get_next_token(jobs_or_other);
					if (j_tok == NULL)
					{
						p_nice(p,pr);
					} // perror invalid input
					free_tokenizer(jobs_or_other);
				} // else 
				
				continue;
			} // else with perrror
		}

		// handle fg commands
		if (strcmp(j_tok, "fg") == 0) {

			free(j_tok);
			j_tok = get_next_token(jobs_or_other);

			struct Job* traveler = head;
			struct Job* candidate = NULL;

			int is_stopped = 0;

			// argument passed to fg
			if (j_tok != NULL) {
				int fg_arg = atoi(j_tok);
				// if second argument to fg is not an integer, reprompt
				if (fg_arg == 0) {
					printf("Invalid: fg: %s: no such job\n", j_tok);
					free(j_tok);
					free_tokenizer(jobs_or_other);
					continue;
				}

				// find the job as specified in the argument 
				while (traveler != NULL) {
					if (traveler->current_job_number == fg_arg) {
						candidate = traveler;
						break;
					}
					traveler = traveler->next;
				}

				// job with number from argument was not found
				if (candidate == NULL) {
					printf("Invalid: fg: %s: no such job\n", j_tok);
					free(j_tok);
					free_tokenizer(jobs_or_other);
					continue;
				}

				if (candidate->status == J_STOPPED) {
					printf("Restarting: ");
					is_stopped = 0;
				}
				printf("%s\n", candidate->user_input);

			} else { // no argument passed to fg
				// find most recently stopped job, if any jobs are stopped
				int highest_counter = -1;
				while (traveler != NULL) {
					if (traveler->status == J_STOPPED) {
						if (traveler->last_modified_counter > highest_counter) {
							highest_counter = traveler->last_modified_counter;
							candidate = traveler;
						}
					}
					traveler = traveler->next;
				}
				// if there are no stopped jobs, find most recently modified background job
				if (candidate == NULL) {
					traveler = head;
					while (traveler != NULL) {
						if (traveler->last_modified_counter > highest_counter) {
							highest_counter = traveler->last_modified_counter;
							candidate = traveler;
						}
						traveler = traveler->next;
					}
				} else {
					is_stopped = 1;
				}

				// if there are no background jobs running, fg is invalid
				if (candidate == NULL) {
					printf("Invalid: fg: current: no such job\n");
					free(j_tok);
					free_tokenizer(jobs_or_other);
					continue;
				}
				//printe restarting if candidate
				if (is_stopped && candidate->bool_type == BG) {
					printf("Restarting: ");
				}
				printf("%s\n", candidate->user_input);
			}

			// update current job to be the specified job
			current_job = candidate;
			current_job->status = J_RUNNING;
			current_job->bool_type = FG;
			current_job->last_modified_counter = job_counter;
			job_counter++;
			pid = current_job->pid;
			// give terminal control
			pcb * actual;
			actual = find_PCB_pid(get_neg(), pid);
			if (actual == NULL){
				actual = find_PCB_pid(get_one(), pid);
				if (actual == NULL){
					actual = find_PCB_pid(get_zero(),pid);
				}
			}
			if (actual == NULL){
				printf("ERRROR CANT FIND CANDIDATE FOR FG");
				continue;
			}
			actual->ground = FG;
			// }
			// if (actual != NULL){
			// 	set_prev_fg_pcb(actual);
			// } else{
			// 	printf("ERROR: PCB cannot be found\n");
			// 	continue;
			// }

			// if job is a stopped job that has been continued, send SIGCONT
			if (is_stopped) {
				p_kill(current_job->pid, S_SIGCONT);
				actual->changed = FALSE;

			}


			int bool_signal = FALSE;
			//for (int i = 0; i < current_job->num_processes; i++) {
			status = -1;
			// TEMPORRARY BROKEN --> HOW TF DO WE WAIT ON SPECIFIC PID
			// nvm my brain jsut fucking nutted
			int found_fg = FALSE;
			// p_wait but hangind
			p_wait_struct * wait_fg = p_wait(HANG, current_job->pid);
			if (wait_fg == NULL){
				// no bg to wait on
				printf("OOF\n");
				//send error
				break;
			}
			status = wait_fg->status;
			//hit our target hanging candidtae
			found_fg = TRUE;
			if (W_WIFSTOPPED(status)) {
				// Make job BG
				// Make job stopped
				// print (Stopped)
				// break the loop
				current_job->bool_type = BG;
				current_job->status = J_STOPPED;
				current_job->last_modified_counter = job_counter;
				job_counter++;
				pcb * actual2;
				actual2 = find_PCB_pid(get_neg(), pid);
				if (actual2 == NULL){
					actual2 = find_PCB_pid(get_one(), pid);
					if (actual2 == NULL){
						actual2 = find_PCB_pid(get_zero(),pid);
					}
				}
				if (actual2 == NULL){
					printf("ERRROR CANT FIND CANDIDATE FOR FG");
					continue;
				}
				actual2->ground = BG;
				printf("\nStopped: %s\n", current_job->user_input);
			}

			if (W_WIFEXITED(status) || W_WIFSIGNALED(status)) {
				// print exited
				// update pid of finishedlist in job struct
				// find index of current pid
				// change the index of finish_pids[index] to TRUE
				remove_job_index(head, current_job->current_job_number);
				current_job = NULL;
				printf("\n");
			}
			// free memory and reprompt user
			free(j_tok);
			free_tokenizer(jobs_or_other);
			continue;
		}
		// handle bg commands
		if (strcmp(j_tok, "bg") == 0) { 
			current_job = NULL;
			free(j_tok);
			//linked list is empty
			if (head == NULL){
				printf("Invalid: No jobs exist.\n");
				free_tokenizer(jobs_or_other);
				continue;
			}
			// check if number specified in next arg
			j_tok = get_next_token(jobs_or_other);
			// no number specified
			if (j_tok == NULL) {
				// go thorugh LL to find most recent stopped job; if none found, return nothing
				struct Job * bg_job = head;
				while (bg_job != NULL){
					if (bg_job->status== J_STOPPED && bg_job->bool_type == BG){
						if (current_job == NULL){
							current_job = bg_job;
						} else if (current_job->last_modified_counter < bg_job->last_modified_counter){
							current_job = bg_job;
						}
					}
					struct Job * next_job = bg_job->next;
					bg_job = next_job;
					
				}
				if (current_job == NULL){
					printf("Invalid : No jobs that can be ran in bg. BG jobs already running and or no stopped jobs.\n");
					free_tokenizer(jobs_or_other);
					free(j_tok);
					continue;
				} else {
					// set the selected bg stopped process to running
					current_job->status = J_RUNNING;
					current_job->last_modified_counter = job_counter;
					job_counter ++;
					p_kill(current_job->pid, S_SIGCONT);
					pcb * actual2;
					actual2 = find_PCB_pid(get_neg(), current_job->pid);
					if (actual2 == NULL){
						actual2 = find_PCB_pid(get_one(), current_job->pid);
						if (actual2 == NULL){
							actual2 = find_PCB_pid(get_zero(),current_job->pid);
						}
					}
					if (actual2 == NULL){
						printf("ERRROR CANT FIND CANDIDATE FOR BG");
						continue;
					}
					// do not want immediate trigger
					actual2->changed = FALSE;

					// print running message without amp
					char * command_tok;
					char command_cpy[1024];
					memset(command_cpy, '\0',  1024 * sizeof(char));
					strcpy(command_cpy, current_job->user_input);
					command_tok = strtok(command_cpy,"&");
					// make sure to trim last element in 
					printf("Running: %s\n", command_tok);
				}

			} else {
				// number specified
				int specific_job = atoi(j_tok);
				if (specific_job == 0){
					//something went wrong with atoi so throw error
					printf("Invalid: Error in bg input\n" );
					free(j_tok);
					free_tokenizer(jobs_or_other);
					continue;
				}
				// go through LL to find matching job number and set it to current job
				// if running in bg say its running; if doesnt exist return nothing or error message
				struct Job * bg_job = head;
				while (bg_job != NULL){
					if (bg_job->current_job_number == specific_job){
						current_job = bg_job;
						//found_bg = TRUE;
						break;
					}
					struct Job * new_bg_job = bg_job->next;
					bg_job = new_bg_job;
				}
				if (bg_job == NULL){
					//nothing found with that job number
					//return error
					printf("Invalid: No stopped job with that number\n" );
					free(j_tok);
					free_tokenizer(jobs_or_other);
					continue;
				}
				if(current_job->status == J_RUNNING){
					// bg process already running
					printf("Job already running in bg\n" );
					free(j_tok);
					free_tokenizer(jobs_or_other);
					continue;
				}
				if (current_job->status == J_STOPPED){
					// bg needs tom move from stopped to runnning in BG
					current_job->status = J_RUNNING;
					current_job->last_modified_counter = job_counter;
					job_counter ++;
					// restart process in bg
					p_kill(current_job->pid, S_SIGCONT);
					pcb * actual2;
					actual2 = find_PCB_pid(get_neg(), current_job->pid);
					if (actual2 == NULL){
						actual2 = find_PCB_pid(get_one(), current_job->pid);
						if (actual2 == NULL){
							actual2 = find_PCB_pid(get_zero(),current_job->pid);
						}
					}
					if (actual2 == NULL){
						printf("ERRROR CANT FIND CANDIDATE FOR BG");
						continue;
					}
					// do not want immediate trigger
					actual2->changed = FALSE;

					// print running message
					char * command_tok;
					char command_cpy[1024];
					memset(command_cpy, '\0',  1024 * sizeof(char));
					strcpy(command_cpy, current_job->user_input);
					command_tok = strtok(command_cpy,"&");
					// make sure to trim last element in 
					printf("Running: %s\n", command_tok);
				}
				free(j_tok);
				free_tokenizer(jobs_or_other);
				continue;
			}
			free(j_tok);
			free_tokenizer(jobs_or_other);
			continue;
		}
		free(j_tok);
		free_tokenizer(jobs_or_other);



		// NOTE: ADD SUPPORT FOR >>
		TOKENIZER * cli_tks = init_tokenizer(user_input);
		char * c_tok = get_next_token(cli_tks);
		char ** cli_args;
		char * input_file;
		char * output_file;
		int c_count;
		// 5 cases for redirects
		if (INPUT_SEEN == FALSE && OUTPUT_SEEN == FALSE && APPEND_SEEN == FALSE) {
			// WORKS
			// 1: no input, no output
			c_count = 0;
			while (c_tok != NULL && strcmp(c_tok, "&") != 0) {
				c_count++;
				free(c_tok);
				c_tok = get_next_token(cli_tks);
			}

			free_tokenizer(cli_tks);
			free(c_tok);

			cli_args = malloc(c_count * sizeof(char *));

			cli_tks = init_tokenizer(user_input);
			c_tok = get_next_token(cli_tks);

			int cli_index = 0;
			while (c_tok != NULL && cli_index < c_count) {
				cli_args[cli_index] = c_tok;
				c_tok = get_next_token(cli_tks);
				cli_index++;
			}
			if (c_tok != NULL) free(c_tok);
			free_tokenizer(cli_tks);
		} else if (INPUT_SEEN == TRUE && OUTPUT_SEEN == FALSE && APPEND_SEEN == FALSE) {
			// 
			// 2: input, no output
			c_count = 0;
			while (c_tok != NULL && strcmp(c_tok, "<") != 0) {
				c_count++;
				free(c_tok);
				c_tok = get_next_token(cli_tks);
			}

			if (strcmp(c_tok, "<") == 0) {
				free(c_tok);
				c_tok = get_next_token(cli_tks);
				input_file = c_tok;
			}

			free_tokenizer(cli_tks);

			cli_args = malloc(c_count * sizeof(char *));

			cli_tks = init_tokenizer(user_input);
			c_tok = get_next_token(cli_tks);

			int cli_index = 0;
			while (c_tok != NULL) {
				cli_args[cli_index] = c_tok;
				c_tok = get_next_token(cli_tks);
				cli_index++;
			}
			if (c_tok != NULL) free(c_tok);
			// input_file is the input file
			// cli_args is all user input tokens before "<"
		} else if (INPUT_SEEN == FALSE && OUTPUT_SEEN == TRUE) {
			// 3: no input, output
			c_count = 0;
			while (c_tok != NULL && strcmp(c_tok, ">") != 0) {
				c_count++;
				free(c_tok);
				c_tok = get_next_token(cli_tks);
			}

			if (strcmp(c_tok, ">") == 0) {
				free(c_tok);
				c_tok = get_next_token(cli_tks);
				output_file = c_tok;
			}

			free_tokenizer(cli_tks);

			cli_args = malloc(c_count * sizeof(char *));

			cli_tks = init_tokenizer(user_input);
			c_tok = get_next_token(cli_tks);

			int cli_index = 0;
			while (c_tok != NULL && strcmp(c_tok, ">") != 0) {
				cli_args[cli_index] = c_tok;
				c_tok = get_next_token(cli_tks);
				cli_index++;
			}
			if (c_tok != NULL) free(c_tok);
			// free_tokenizer(cli_tks);
			// output_file is the output file
			// cli_args is all user input tokens before ">"
		} else if (INPUT_SEEN == TRUE && OUTPUT_SEEN == TRUE) {
			// 4: input, output

			c_count = 0;
			while (c_tok != NULL && strcmp(c_tok, ">") != 0 && strcmp(c_tok, "<") != 0) {
				c_count++;
				free(c_tok);
				c_tok = get_next_token(cli_tks);
			}

			if (strcmp(c_tok, ">") == 0) {
				free(c_tok);
				c_tok = get_next_token(cli_tks);
				output_file = c_tok;

				c_tok = get_next_token(cli_tks);
				while (strcmp(c_tok, "<") != 0) {
					free(c_tok);
					c_tok = get_next_token(cli_tks);
				}

				free(c_tok);
				c_tok = get_next_token(cli_tks);
				input_file = c_tok;
				free_tokenizer(cli_tks);
			} else if (strcmp(c_tok, "<") == 0) {
				free(c_tok);
				c_tok = get_next_token(cli_tks);
				input_file = c_tok;

				c_tok = get_next_token(cli_tks);
				while (strcmp(c_tok, ">") != 0) {
					free(c_tok);
					c_tok = get_next_token(cli_tks);
				}

				free(c_tok);
				c_tok = get_next_token(cli_tks);
				output_file = c_tok;
				free_tokenizer(cli_tks);
			}

			cli_args = malloc(c_count * sizeof(char *));

			cli_tks = init_tokenizer(user_input);
			c_tok = get_next_token(cli_tks);

			int cli_index = 0;
			while (c_tok != NULL && strcmp(c_tok, ">") != 0 && strcmp(c_tok, "<") != 0) {
				cli_args[cli_index] = c_tok;
				cli_index++;
				c_tok = get_next_token(cli_tks);
			}
			free(c_tok);
			free_tokenizer(cli_tks);
			// output_file is the output file
			// input_file is the input file
			// cli_args is all user input tokens after "<" and before ">"
		} else if (INPUT_SEEN == FALSE && APPEND_SEEN == TRUE) {
			// 3: no input, append
			c_count = 0;
			while (c_tok != NULL && strcmp(c_tok, ">") != 0) {
				c_count++;
				free(c_tok);
				c_tok = get_next_token(cli_tks);
			}

			if (strcmp(c_tok, ">") == 0) {
				free(c_tok);
				c_tok = get_next_token(cli_tks);
				free(c_tok);
				c_tok = get_next_token(cli_tks);
				output_file = c_tok;
			}

			free_tokenizer(cli_tks);

			cli_args = malloc(c_count * sizeof(char *));

			cli_tks = init_tokenizer(user_input);
			c_tok = get_next_token(cli_tks);

			int cli_index = 0;
			while (c_tok != NULL && strcmp(c_tok, ">") != 0) {
				cli_args[cli_index] = c_tok;
				c_tok = get_next_token(cli_tks);
				cli_index++;
			}
			if (c_tok != NULL) free(c_tok);
			// free_tokenizer(cli_tks);
			// output_file is the output file
			// cli_args is all user input tokens before ">"
		} else {
			// 4: input true, append true

			c_count = 0;
			while (c_tok != NULL && strcmp(c_tok, ">") != 0 && strcmp(c_tok, "<") != 0) {
				c_count++;
				free(c_tok);
				c_tok = get_next_token(cli_tks);
			}

			if (strcmp(c_tok, ">") == 0) {
				free(c_tok);
				c_tok = get_next_token(cli_tks);
				free(c_tok);
				c_tok = get_next_token(cli_tks);
				output_file = c_tok;

				c_tok = get_next_token(cli_tks);
				while (strcmp(c_tok, "<") != 0) {
					free(c_tok);
					c_tok = get_next_token(cli_tks);
				}

				free(c_tok);
				c_tok = get_next_token(cli_tks);
				input_file = c_tok;
				free_tokenizer(cli_tks);
			} else if (strcmp(c_tok, "<") == 0) {
				free(c_tok);
				c_tok = get_next_token(cli_tks);
				input_file = c_tok;

				c_tok = get_next_token(cli_tks);
				while (strcmp(c_tok, ">") != 0) {
					free(c_tok);
					c_tok = get_next_token(cli_tks);
				}

				free(c_tok);
				c_tok = get_next_token(cli_tks);
				free(c_tok);
				c_tok = get_next_token(cli_tks);
				output_file = c_tok;
				free_tokenizer(cli_tks);
			}

			cli_args = malloc(c_count * sizeof(char *));

			cli_tks = init_tokenizer(user_input);
			c_tok = get_next_token(cli_tks);

			int cli_index = 0;
			while (c_tok != NULL && strcmp(c_tok, ">") != 0 && strcmp(c_tok, "<") != 0) {
				cli_args[cli_index] = c_tok;
				cli_index++;
				c_tok = get_next_token(cli_tks);
			}
			free(c_tok);
			free_tokenizer(cli_tks);
			// output_file is the output file
			// input_file is the input file
			// cli_args is all user input tokens after "<" and before ">"
		}
		
		// TESTING REDIRECTION
		// printf("iter count: %d\n", c_count);
		// printf("output_file: %s\n", output_file);
		// printf("input_file: %s\n", input_file);
		// for (int i = 0; i < c_count; i++) {
		// 	printf("%s ", cli_args[i]);
		// }
		// printf("\nDONE\n");
		// __________WE SPAWNING DEM CREEPERS________________
		// char * yinger[4] = {"dummy6", "ARG1", "ARG2", "ARG3"};

		void* user_function;
		if (c_count > 0) {
			if (strcmp(cli_args[0], "ls") == 0) {
				user_function = f_ls;
				if (c_count != 1) {
					p_perror("ls");
					continue;
				}
			} else if (strcmp(cli_args[0], "cat") == 0) {
				user_function = f_cat;
				if (c_count != 1) {
					p_perror("cat");
					continue;
				}
			} else if (strcmp(cli_args[0], "touch") == 0) {
				user_function = f_touch;
				if (c_count != 2) {
					p_perror("touch");
					continue;
				}
			} else if (strcmp(cli_args[0], "rm") == 0) {
				user_function = f_rm;
				if (c_count != 2) {
					p_perror("rm");
					continue;
				}
			} else if (strcmp(cli_args[0], "busy") == 0) {
				user_function = busy;
				if (c_count != 1) {
					p_perror("busy");
					continue;
				}
			} else if (strcmp(cli_args[0], "history") == 0) {
				user_function = history;
				if (c_count != 1) {
					p_perror("history");
					continue;
				}
			} else if (strcmp(cli_args[0], "ps") == 0) {
				user_function = ps;
				if (c_count != 1) {
					p_perror("ps");
					continue;
				}
			} else if (strcmp(cli_args[0], "zombify") == 0) {
				user_function = zombify;
				if (c_count != 1) {
					p_perror("zombify");
					continue;
				}
			}else if (strcmp(cli_args[0], "orphanify") == 0) {
				user_function = orphanify;
				if (c_count != 1) {
					p_perror("history");
					continue;
				}
			}else if (strcmp(cli_args[0], "sleep") == 0) {
				if (c_count != 2 || atoi(cli_args[1]) == 0) {
					p_perror("sleep");
					continue;
				}
				user_function = _sleep;
			} else {
				f_write(F_STDOUT, "No such file or directory\n", 27);
				continue;
			}

		} else {
			continue;
		}
		
		// pid = p_spawn(user_function, c_count, cli_args, input_file, output_file,
		// 			APPEND_SEEN, amper_found ? BG : FG); 

		//p_nice(pid, -1);
		pid = p_spawn(user_function, c_count, cli_args, amper_found ? BG : FG);
		// opening file descriptors
		// has in redirect
		if (input_file != NULL) {
			int in_fd = f_open(input_file, F_READ);
			if (in_fd == F_FAILED) {
				p_perror("in redirect");
			}
			f_dup2(in_fd, F_STDIN);
		}

		//has out redirect
		if (output_file != NULL) {
			int mode = APPEND_SEEN ? F_APPEND : F_WRITE;
			int out_fd = f_open(output_file, mode);
			if (out_fd == F_FAILED) {
				p_perror("out redirect");
			}
			f_dup2(out_fd, F_STDOUT);
		}

		// char * yinger[4] = {"dummy6", "ARG1", "ARG2", "ARG3"};
		
		// pid = p_spawn(dummy6, 4, yinger, input_file, output_file, 
		//  		APPEND_SEEN, amper_found ? BG : FG);
		struct Job * new_job;
		if (amper_found == TRUE) {
			new_job = create_job(pid, BG, job_counter, user_input);
		} else {
			new_job = create_job(pid, FG, job_counter, user_input);
			//terminal_control = pid;
		}
		head = add_job(head, new_job);
		current_job = new_job;
		job_counter ++;

		
		if (!amper_found){

			/*
			if (get_zero() != NULL) {
				set_prev_fg_pcb(find_PCB_pid(get_zero(), pid));
			}
			else {
				set_prev_fg_pcb(get_cur_pcb());
			}
			*/

			p_wait_struct * ws = p_wait(HANG, pid);

			// job terminates so we just remove it form job queue
			if (W_WIFEXITED(ws->status) || W_WIFSIGNALED(ws->status)){
				remove_job_index(head, current_job->current_job_number);
				current_job = NULL;
			}
			if (W_WIFSTOPPED(ws->status)){
				current_job->bool_type = BG;
				current_job->status = J_STOPPED;
				current_job->last_modified_counter = job_counter;
				job_counter++;
				pcb * actual;
				actual = find_PCB_pid(get_neg(), pid);
				if (actual == NULL){
					actual = find_PCB_pid(get_one(), pid);
					if (actual == NULL){
						actual = find_PCB_pid(get_zero(),pid);
					}
				}
				if (actual == NULL){
					printf("ERRROR CANT FIND CANDIDATE FOR FG");
					continue;
				}
				actual->ground = BG;
				printf("\nStopped: %s\n", current_job->user_input);
			}
			set_prev_fg_pcb(get_shell_pcb());
		} else{
			set_prev_fg_pcb(get_shell_pcb());
		}

		//change after

		set_prev_fg_pcb(get_shell_pcb());
	}
}

void run_os(){
	char * shell_com[1] = {"shell"};
	pid = p_spawn(shell, 1, shell_com, 1);
	p_nice(pid, -1);
	set_shell_pcb(get_neg());
	set_prev_fg_pcb(get_neg());
	setcontext(get_scheduler_context());
	// write on
	//0 is NOHANG
}

int main(int argc, char const *argv[]) {
	setup_file();
	setup_timer();
	setup_scheduler();
	setup_signals();
	setup_idle();
	set_signal_stack();
	init_filesystem("flatfat");

	run_os();

	return 0;
}

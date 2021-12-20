#include <string.h>
#include <stdlib.h> 
#include <stdio.h>

//struct Job;

struct Job {
	int pid;
	int current_job_number;
	int bool_type; // for background or forground
	int last_modified_counter;
	struct Job * next;
	char * user_input;
	int status; // tell whether its running or stopped
	int num_processes;
};


struct Job * create_job(int given_pid, int ground, int counter, char * input );

void free_job(struct Job * j);

void update_status(struct Job * job, int status, int counter);

void update_ground_type(struct Job * job, int type, int counter);

struct Job * add_job(struct Job * head, struct Job * new_job);

struct Job * remove_job_index(struct Job * head, int num);

void print_job(struct Job* job);


#define TRUE 1
#define FALSE 0
#define FG 1
#define BG 0
#define J_RUNNING 0
#define J_STOPPED 1
#define J_DONE 2
/* Multidup
 *
 * One-to-many file duplication tool
 *
 * Copyright (c) 2018-2019 Alec Hitchiner
 */
#include "dup_worker.h"

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(int argc, char **argv){
	dup_worker *workers;
	int is_done, is_error;
	pthread_cond_t status_condition;
	pthread_mutex_t status_mutex;
	pthread_t *worker_threads;
	size_t i, j;

	/* Check argument count */
	if(argc < 3){
		printf("Multidup: A one-to-many file duplication tool\n");
		printf("Copyright (c) 2018-2019 Alec Hitchiner\n");
		printf("Usage: %s [input file] [output file] {[output file]...}\n",
				argv[0]);
		return -1;
	}

	/* Create status update mutex and condition variable */
	pthread_mutex_init(&status_mutex, NULL);
	pthread_cond_init(&status_condition, NULL);

	/* Configure worker threads */
	workers = malloc((argc - 2) * sizeof(dup_worker));
	for(i = 0; i < (size_t)(argc - 2); i++){
		workers[i].input_fname = argv[1];
		workers[i].output_fname = argv[2 + i];
		workers[i].progress = 0;
		workers[i].status_condition = &status_condition;
		workers[i].status_mutex = &status_mutex;
		workers[i].state = WORKER_NOT_STARTED;
	}

	/* Create worker threads */
	worker_threads = malloc((argc - 2) * sizeof(pthread_t));
	for(i = 0; i < (size_t)(argc - 2); i++){
		pthread_create(&(worker_threads[i]),
				NULL,
				dup_worker_thread,
				&(workers[i]));
	}

	/* Acquire status updating mutex */
	pthread_mutex_lock(&status_mutex);

	/* Start display */
	do{
		/* Initialize possible states */
		is_done = 1;
		is_error = 0;

		/* Clear screen */
		printf("\033[H\033[J");

		/* Process each worker */
		for(i = 0; i < (size_t)(argc - 2); i++){
			printf("%21s: ", workers[i].output_fname);
			switch(workers[i].state){
				case WORKER_NOT_STARTED:
					printf("Starting\n");
					is_done = 0;
					break;
				case WORKER_ERROR:
					printf("ERROR - %s\n", workers[i].err_msg);
					is_error = 1;
					break;
				case WORKER_WORKING:
					/* Progress bar */
					printf("%u%% [", workers[i].progress);
					for(j = 0; j < 50; j++){
						if(j < (workers[i].progress / 2)){
							printf("#");
						} else{
							printf(" ");
						}
					}
					printf("]\n");
					is_done = 0;
					break;
				case WORKER_DONE:
					printf("Done!\n");
					break;
				default:
					printf("This should never appear\n");
					break;
			}
		}

		if(!is_done){
			/* Wait for some thread to have a status update */
			pthread_cond_wait(&status_condition, &status_mutex);
		}
	}while(!is_done);

	/* Release status updating mutex */
	pthread_mutex_unlock(&status_mutex);

	/* Synchronize */
	printf("Synchronizing disks...\n");
	sync();

	/* Print error statement */
	if(is_error){
		printf("Errors ocurred, not all destinations may be valid.\n");
	}

	/* Clean up */
	for(i = 0; i < (size_t)(argc - 2); i++){
		pthread_join(worker_threads[i], NULL);
	}
	free(workers);
	free(worker_threads);

	return 0;
}

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

	/* Configure worker threads */
	workers = malloc((argc - 2) * sizeof(dup_worker));
	for(i = 0; i < (size_t)(argc - 2); i++){
		workers[i].input_fname = argv[1];
		workers[i].output_fname = argv[2 + i];
		workers[i].progress = 0;
		pthread_mutex_init(&(workers[i].mutex), NULL);
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

	/* Start display */
	do{
		/* Update every 5 seconds */
		sleep(5);

		/* Initialize possible states */
		is_done = 1;
		is_error = 0;

		/* Clear screen */
		printf("\033[H\033[J");

		/* Process each worker */
		for(i = 0; i < (size_t)(argc - 2); i++){
			pthread_mutex_lock(&(workers[i].mutex));
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
			pthread_mutex_unlock(&(workers[i].mutex));
		}
	}while(!is_done);

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

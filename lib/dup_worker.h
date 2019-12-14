/* Multidup
 *
 * One-to-many file duplication tool
 *
 * Copyright (c) 2018-2019 Alec Hitchiner
 */
#ifndef DUP_WORKER_H
#define DUP_WORKER_H

#include <pthread.h>

#define BLOCK_SIZE 4096

typedef enum {
	WORKER_NOT_STARTED,
	WORKER_ERROR,
	WORKER_WORKING,
	WORKER_DONE
} worker_state;

typedef struct {
	char *input_fname;
	char *output_fname;
	unsigned int progress;
	pthread_mutex_t mutex;
	worker_state state;
	int errnum;
	char err_msg[49];
} dup_worker;

void *dup_worker_thread(void *worker);

#endif /* Include Guard */

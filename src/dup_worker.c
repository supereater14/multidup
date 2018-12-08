/* Multidup
 *
 * One-to-many file duplication tool
 *
 * Copyright (c) 2018 Alec Hitchiner
 */
#include "dup_worker.h"

#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void *dup_worker_thread(void *worker){
	char buf[BLOCK_SIZE];
	dup_worker *worker_data;
	int infile, outfile;
	off_t input_size, check_off;
	ssize_t next_percent, step_size;
	ssize_t data_read, data_written, data_waiting, total_written;
	struct stat infile_stat;

	/* Get worker data structure */
	worker_data = (dup_worker*)worker;

	/* Open input file */
	pthread_mutex_lock(&worker_data->mutex);
	infile = open(worker_data->input_fname, O_RDONLY);
	pthread_mutex_unlock(&worker_data->mutex);
	if(infile < 0){
		pthread_mutex_lock(&worker_data->mutex);
		worker_data->state = WORKER_ERROR;
		pthread_mutex_unlock(&worker_data->mutex);
		return NULL;
	}

	/* Get input file mode so that output can match */
	if(fstat(infile, &infile_stat) < 0){
		pthread_mutex_lock(&worker_data->mutex);
		worker_data->state = WORKER_ERROR;
		pthread_mutex_unlock(&worker_data->mutex);
	}

	/* Open output file */
	pthread_mutex_lock(&worker_data->mutex);
	outfile = open(worker_data->output_fname,
			O_WRONLY | O_CREAT,
			infile_stat.st_mode);
	pthread_mutex_unlock(&worker_data->mutex);
	if(outfile < 0){
		pthread_mutex_lock(&worker_data->mutex);
		worker_data->state = WORKER_ERROR;
		pthread_mutex_unlock(&worker_data->mutex);
		close(infile);
		return NULL;
	}

	/* Find input file length */
	input_size = lseek(infile, 0, SEEK_END);
	if(input_size < 0){
		pthread_mutex_lock(&worker_data->mutex);
		worker_data->state = WORKER_ERROR;
		pthread_mutex_unlock(&worker_data->mutex);
		close(infile);
		close(outfile);
		return NULL;
	}

	/* Seek back to start of file */
	check_off = lseek(infile, 0, SEEK_SET);
	if(check_off < 0){
		pthread_mutex_lock(&worker_data->mutex);
		worker_data->state = WORKER_ERROR;
		pthread_mutex_unlock(&worker_data->mutex);
		close(infile);
		close(outfile);
		return NULL;
	}

	/* Find percent offsets */
	step_size = input_size / 100;
	next_percent = step_size;

	/* Start duplicating */
	data_read = 0;
	total_written = 0;
	do{
		/* Read block of data */
		data_read = read(infile, buf, BLOCK_SIZE);
		if(data_read < 0){
			pthread_mutex_lock(&worker_data->mutex);
			worker_data->state = WORKER_ERROR;
			pthread_mutex_unlock(&worker_data->mutex);
			close(infile);
			close(outfile);
			return NULL;
		}

		/* Write block of data */
		data_waiting = data_read;
		pthread_mutex_lock(&worker_data->mutex);
		worker_data->state = WORKER_WORKING;
		pthread_mutex_unlock(&worker_data->mutex);
		do{
			data_written = write(outfile,
					buf + (data_read - data_waiting),
					data_waiting);
			if(data_written < 0){
				pthread_mutex_lock(&worker_data->mutex);
				worker_data->state = WORKER_ERROR;
				pthread_mutex_unlock(&worker_data->mutex);
				close(infile);
				close(outfile);
				return NULL;
			}
			data_waiting -= data_written;
		} while(data_waiting > 0);

		/* Increment total read */
		total_written += data_read;

		/* Advance progress */
		if(total_written >= next_percent){
			pthread_mutex_lock(&worker_data->mutex);
			worker_data->progress++;
			pthread_mutex_unlock(&worker_data->mutex);
			next_percent += step_size;
		}
	} while(data_read > 0);

	/* Clean up */
	close(infile);
	close(outfile);

	/* Finish */
	pthread_mutex_lock(&worker_data->mutex);
	worker_data->state = WORKER_DONE;
	pthread_mutex_unlock(&worker_data->mutex);

	return NULL;
}

/* Multidup
 *
 * One-to-many file duplication tool
 *
 * Copyright (c) 2018-2019 Alec Hitchiner
 */
#include "dup_worker.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

void *dup_worker_thread(void *worker){
	char buf[BLOCK_SIZE];
	dup_worker *worker_data;
	int infile, outfile;
	off_t input_size, check_off;
	off_t next_percent, step_size, total_written;
	ssize_t data_read, data_written, data_waiting;
	struct stat infile_stat;

	/* Get worker data structure */
	worker_data = (dup_worker*)worker;

	/* Open input file */
	infile = open(worker_data->input_fname, O_RDONLY);
	if(infile < 0){
		pthread_mutex_lock(worker_data->status_mutex);
		worker_data->state = WORKER_ERROR;
		worker_data->errnum = errno;
		strerror_r(errno, worker_data->err_msg, 49);
		pthread_cond_signal(worker_data->status_condition);
		pthread_mutex_unlock(worker_data->status_mutex);
		return NULL;
	}

	/* Get input file mode so that output can match */
	if(fstat(infile, &infile_stat) < 0){
		pthread_mutex_lock(worker_data->status_mutex);
		worker_data->state = WORKER_ERROR;
		worker_data->errnum = errno;
		strerror_r(errno, worker_data->err_msg, 49);
		pthread_cond_signal(worker_data->status_condition);
		pthread_mutex_unlock(worker_data->status_mutex);
	}

	/* Open output file */
	outfile = open(worker_data->output_fname,
			O_WRONLY | O_CREAT,
			infile_stat.st_mode);
	if(outfile < 0){
		pthread_mutex_lock(worker_data->status_mutex);
		worker_data->state = WORKER_ERROR;
		worker_data->errnum = errno;
		strerror_r(errno, worker_data->err_msg, 49);
		pthread_cond_signal(worker_data->status_condition);
		pthread_mutex_unlock(worker_data->status_mutex);
		close(infile);
		return NULL;
	}

	/* Find input file length */
	input_size = lseek(infile, 0, SEEK_END);
	if(input_size < 0){
		pthread_mutex_lock(worker_data->status_mutex);
		worker_data->state = WORKER_ERROR;
		worker_data->errnum = errno;
		strerror_r(errno, worker_data->err_msg, 49);
		pthread_cond_signal(worker_data->status_condition);
		pthread_mutex_unlock(worker_data->status_mutex);
		close(infile);
		close(outfile);
		return NULL;
	}

	/* Seek back to start of file */
	check_off = lseek(infile, 0, SEEK_SET);
	if(check_off < 0){
		pthread_mutex_lock(worker_data->status_mutex);
		worker_data->state = WORKER_ERROR;
		worker_data->errnum = errno;
		strerror_r(errno, worker_data->err_msg, 49);
		pthread_cond_signal(worker_data->status_condition);
		pthread_mutex_unlock(worker_data->status_mutex);
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
	pthread_mutex_lock(worker_data->status_mutex);
	worker_data->state = WORKER_WORKING;
	pthread_cond_signal(worker_data->status_condition);
	pthread_mutex_unlock(worker_data->status_mutex);
	do{
		/* Read block of data */
		data_read = read(infile, buf, BLOCK_SIZE);
		if(data_read < 0){
			pthread_mutex_lock(worker_data->status_mutex);
			worker_data->state = WORKER_ERROR;
			worker_data->errnum = errno;
			strerror_r(errno, worker_data->err_msg, 49);
			pthread_cond_signal(worker_data->status_condition);
			pthread_mutex_unlock(worker_data->status_mutex);
			close(infile);
			close(outfile);
			return NULL;
		}

		/* Write block of data */
		data_waiting = data_read;
		do{
			data_written = write(outfile,
					buf + (data_read - data_waiting),
					data_waiting);
			if(data_written < 0){
				pthread_mutex_lock(worker_data->status_mutex);
				worker_data->state = WORKER_ERROR;
				worker_data->errnum = errno;
				strerror_r(errno, worker_data->err_msg, 49);
				pthread_cond_signal(worker_data->status_condition);
				pthread_mutex_unlock(worker_data->status_mutex);
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
			pthread_mutex_lock(worker_data->status_mutex);
			worker_data->progress++;
			pthread_cond_signal(worker_data->status_condition);
			pthread_mutex_unlock(worker_data->status_mutex);
			next_percent += step_size;
		}
	} while(data_read > 0);

	/* Clean up */
	close(infile);
	close(outfile);

	/* Finish */
	pthread_mutex_lock(worker_data->status_mutex);
	worker_data->state = WORKER_DONE;
	pthread_mutex_unlock(worker_data->status_mutex);

	return NULL;
}

#ifndef __DISK_MGR__
#define __DISK_MGR__

#include "ppos_data.h"

typedef enum
{
	READ,
	WRITE
} operations;

typedef struct request_t
{
	struct request_t *next, *prev; // works as a circular queue
	task_t *task;				   // Task: The task that requested the operation
	operations operation;		   // Operation: Which operation was requested(READ or WRITE)
	int block;					   // Block: In which block it is the data that should be modified(WRITE) or retrieved(READ)
	void *buffer;				   // Buffer: The buffer to put the data retrieved(READ) or modified(WRITE)
} request_t;

typedef struct
{
	request_t *requests;   // Queue of requests: Here is stored the requests operations to the disk
	task_t *queue;		   // Queue of the disk: Here is stored the tasks that requested a operation from disk
	semaphore_t semaphore; // Semaphore of the disk: It is used to prevent racing condition on request operations to the disk
} disk_t;

// Disk initialization, return -1 in case of error, or 0 in case of success
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init(int *numBlocks, int *blockSize);

// Read of a block "block", from the disk to the buffer "buffer"
int disk_block_read(int block, void *buffer);

// Writting of a block "block", from the buffer "buffer" to the disk
int disk_block_write(int block, void *buffer);

#endif
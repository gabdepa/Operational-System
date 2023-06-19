// GRR20197155 Gabriel Razzolini Pires De Paula

// Including libs needed
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include "ppos.h"
#include "ppos_disk.h"
#include "disk.h"
#include "queue.h"

extern task_t *current_task; // Current task in execution
extern task_t *ready_tasks;  // Queue of tasks ready for execution
int disk_signal;             // Disk signal flag
struct sigaction action2;    // Structure to handle signals
disk_t disk;                 // Disk structure
task_t disk_manager;         // Disk manager task structure

// Disk driver function
void diskDriverBody()
{
    // Continuous loop
    while (TRUE)
    {
        // Decrease the semaphore count (lock)
        sem_down(&disk.semaphore);
        // If a disk signal is generated
        if (disk_signal)
        {
            // Get the HEAD task from the disk request(FCFS)
            task_t *task = disk.request->task;
            // Remove the request from the queue
            queue_remove((queue_t **)&disk.requests, (queue_t *)disk.request);
            // Set the task status as ready
            task->status = TASK_READY;
            // Resume the task
            task_resume(task, &disk.queue);
            // Free memory allocated to the request
            free(disk.request);
            // Reset disk signal
            disk_signal = FALSE;
        }
        // Get the status of the disk
        int disk_status = disk_cmd(DISK_CMD_STATUS, 0, 0);
        // If disk is idle and there are requests to be processed
        if ((disk_status == DISK_STATUS_IDLE) && (disk.requests))
        {
            // Get the request from the queue
            disk.request = disk.requests;
            // If operation is write
            if (disk.request->operation == WRITE)
            {
                // Write to disk
                disk_cmd(DISK_CMD_WRITE, disk.request->block, disk.request->buffer);
            }
            // If operation is read
            else if (disk.request->operation == READ)
            {
                // Read from disk
                disk_cmd(DISK_CMD_READ, disk.request->block, disk.request->buffer);
            }
            else // If operation is neither read nor write
            {
                // Print an error message
                perror("ERROR: diskDriverBody()=> Unknown request!\n");
                // Exit with error code
                exit(1);
            }
        }
        // Increase the semaphore count (unlock)
        sem_up(&disk.semaphore);
        // Suspend the disk manager task
        disk_manager.status = TASK_SUSPENDED;
        // Remove the disk manager task from the ready queue
        queue_remove((queue_t **)&ready_tasks, (queue_t *)&disk_manager);
        // Yield the CPU control to the dispatcher task
        task_yield();
    }
}

void handle_signal() // Signal handling function
{
    // Set disk signal
    disk_signal = TRUE;
    // If disk manager task is suspended
    if (disk_manager.status == TASK_SUSPENDED)
    {
        // Add disk manager task to ready queue
        queue_append((queue_t **)&ready_tasks, (queue_t *)&disk_manager);
        // Set disk manager task as ready
        disk_manager.status = TASK_READY;
        return;
    }
    else
    {
        return;
    }
}

int disk_mgr_init(int *numBlocks, int *blockSize)
{
    // If disk initialization fails
    if (disk_cmd(DISK_CMD_INIT, 0, 0))
    {
        // Print an error message
        perror("ERROR: disk_mgr_init()=> Could not initialize disk!\n");
        // Exit with error code
        exit(1);
    }
    // Set the number of blocks
    *numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
    // If setting number of blocks fails
    if (*numBlocks < 0)
    {
        // Print an error message
        perror("ERROR: disk_mgr_init()=> Could not set number of blocks!\n");
        // Exit with error code
        exit(1);
    }
    // Set the size of the blocks
    *blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
    // If setting block size fails
    if (*blockSize < 0)
    {
        // Print an error message
        perror("ERROR: disk_mgr_init()=> Could not set size of the blocks!\n");
        // Exit with error code
        exit(1);
    }
    // Reset disk signal
    disk_signal = FALSE;
    // Initialize the semaphore
    sem_init(&disk.semaphore, 1);
    // Initialize the disk manager task
    task_init(&disk_manager, diskDriverBody, NULL);
    // Set the disk manager task status as suspended
    disk_manager.status = TASK_SUSPENDED;
    /*************************************** SIGACTION CONFIGURATION ****************************************/
    // Set the signal handler function
    action2.sa_handler = handle_signal;
    // Empty the signal mask
    sigemptyset(&action2.sa_mask);
    // Set the signal flags
    action2.sa_flags = 0;
    // If sigaction setup fails
    if (sigaction(SIGUSR1, &action2, 0) < 0)
    {
        // Print an error message
        perror("ERROR: disk_mgr_init()=> Error on sigaction!\n");
        // Exit with error code
        exit(1);
    }
    // Return success
    return 0;
}

// Reads a block of data from the disk
int disk_block_read(int block, void *buf)
{
    // Allocate memory for the request
    request_type *req = malloc(sizeof(request_type));
    // If memory allocation fails
    if (!req)
    {
        // Print an error message
        perror("ERROR: disk_block_read()=> Error on alocatting memory!\n");
        // Exit with error code
        exit(1);
    }
    // Set task attribute to be the current task
    req->task = current_task;
    // Set the operation as READ
    req->operation = READ;
    // Set the buffer
    req->buffer = buf;
    // Set the block
    req->block = block;
    // Set next and previous pointers as NULL
    req->next = req->prev = NULL;
    // Decrease the semaphore count (lock)
    sem_down(&disk.semaphore);
    // Add the request to the queue of requests
    queue_append((queue_t **)&disk.requests, (queue_t *)req);
    // If disk manager task is suspended
    if (disk_manager.status == TASK_SUSPENDED)
    {
        // Set disk manager task as ready
        disk_manager.status = TASK_READY;
        // Add disk manager task to the queue of ready
        queue_append((queue_t **)&ready_tasks, (queue_t *)&disk_manager);
    }
    // Increase the semaphore count (unlock)
    sem_up(&disk.semaphore);
    // Suspend the task
    task_suspend(&disk.queue);
    // Yield the CPU control to the dispatcher task
    task_yield();
    // Return success
    return 0;
}

// Writes a block of data to the disk
int disk_block_write(int block, void *buf)
{
    // Allocate memory for the request
    request_type *req = malloc(sizeof(request_type));
    // If memory allocation fails
    if (!req)
    {
        // Print an error message
        perror("ERROR: disk_block_write()=> Error on alocatting memory!\n");
        // Exit with error code
        exit(1);
    }
    // Set task attribute to be the current task
    req->task = current_task;
    // Set the operation as WRITE
    req->operation = WRITE;
    // Set the buffer
    req->buffer = buf;
    // Set the block
    req->block = block;
    // Set next and previous pointers as NULL
    req->next = req->prev = NULL;
    // Decrease the semaphore count (lock)
    sem_down(&disk.semaphore);
    // Add the request to the queue of requests
    queue_append((queue_t **)&disk.requests, (queue_t *)req);
    // If disk manager task is suspended
    if (disk_manager.status == TASK_SUSPENDED)
    {
        // Set disk manager task as ready
        disk_manager.status = TASK_READY;
        // Add disk manager task to the queue of ready
        queue_append((queue_t **)&ready_tasks, (queue_t *)&disk_manager);
    }
    // Increase the semaphore count (unlock)
    sem_up(&disk.semaphore);
    // Suspend the task
    task_suspend(&disk.queue);
    // Yield the CPU control to the dispatcher task
    task_yield();
    // Return success
    return 0;
}

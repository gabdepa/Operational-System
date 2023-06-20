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
task_t disk_manager;         // Disk manager task

void diskDriverBody()
{
    // Continuous loop
    while (TRUE)
    {
        // Set disk manager task status as RUNNING
        disk_manager.status = TASK_RUNNING;
        // Decrease the semaphore count (lock)
        sem_down(&disk.semaphore);
        // If a disk signal is generated
        if (disk_signal)
        {
            // Set the HEAD task from the disk requests(FCFS) status as ready
            disk.requests->task->status = TASK_READY;
            // Resume the HEAD task from the disk requests(FCFS)
            task_resume(disk.requests->task, &disk.queue);
            // Remove the head request, which was attended, from the requests queue
            queue_remove((queue_t **)&disk.requests, (queue_t *)disk.requests);
            // Reset disk signal
            disk_signal = FALSE;
        }
        // If disk status of the disk is idle and there are requests to be processed
        if ((disk_cmd(DISK_CMD_STATUS, 0, 0) == DISK_STATUS_IDLE) && (disk.requests))
        {
            // If operation for the head of the  queue of requests is write
            if (disk.requests->operation == WRITE)
            {
                // Write to disk
                disk_cmd(DISK_CMD_WRITE, disk.requests->block, disk.requests->buffer);
            }
            // If operation for the head of the  queue of requests
            else if (disk.requests->operation == READ)
            {
                // Read from disk
                disk_cmd(DISK_CMD_READ, disk.requests->block, disk.requests->buffer);
            }
            else // If operation for the head of the  queue of requests is neither read nor write
            {
                // Print an error message
                perror("ERROR: diskDriverBody()=> Unknown request!\n");
                // Exit with error code
                exit(1);
            }
        }
        // Increase the semaphore count (unlock)
        sem_up(&disk.semaphore);
        // Set the disk manager task status as SUSPENDED
        disk_manager.status = TASK_SUSPENDED;
        // Remove the disk manager task from the ready queue
        queue_remove((queue_t **)&ready_tasks, (queue_t *)&disk_manager);
        // Yield the CPU control to the dispatcher task
        task_yield();
    }
}

// Signal handling function
void handle_signal()
{
    // Set disk signal
    disk_signal = TRUE;
    // If disk manager task is suspended
    if (disk_manager.status == TASK_SUSPENDED)
    {
        // Add disk manager task to ready tasks queue
        queue_append((queue_t **)&ready_tasks, (queue_t *)&disk_manager);
        // Set disk manager task status as READY
        disk_manager.status = TASK_READY;
        return;
    }
    else
    {
        // Print an error message
        perror("ERROR: handle_signal()=> Disk manager status is not SUSPENDED, could not handle disk signal now!\n");
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
        exit(-1);
    }
    // Set the number of blocks
    *numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
    // If setting number of blocks fails
    if (*numBlocks < 0)
    {
        // Print an error message
        perror("ERROR: disk_mgr_init()=> Could not set number of blocks!\n");
        // Exit with error code
        exit(-1);
    }
    // Set the size of the blocks
    *blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
    // If setting block size fails
    if (*blockSize < 0)
    {
        // Print an error message
        perror("ERROR: disk_mgr_init()=> Could not set size of the blocks!\n");
        // Exit with error code
        exit(-1);
    }
    // Initialize disk signal(FALSE per default)
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
        exit(-1);
    }
    /*************************************** SIGACTION CONFIGURATION ****************************************/
    // Return success
    return 0;
}

int disk_block_read(int block, void *buffer)
{
    // Allocate memory for the request
    request_t *request = malloc(sizeof(request_t));
    // If memory allocation fails
    if (!request)
    {
        // Print an error message
        perror("ERROR: disk_block_read()=> Error on alocatting memory!\n");
        // Exit with error code
        exit(1);
    }
    // Set next and previous pointers as NULL
    request->next = request->prev = NULL;
    // Set task attribute to be the current task
    request->task = current_task;
    // Set the operation as READ
    request->operation = READ;
    // Set the block
    request->block = block;
    // Set the buffer
    request->buffer = buffer;
    // Decrease the semaphore count (lock)
    sem_down(&disk.semaphore);
    // Add the request to the queue of requests
    queue_append((queue_t **)&disk.requests, (queue_t *)request);
    // If disk manager task is SUSPENDED
    if (disk_manager.status == TASK_SUSPENDED)
    {
        // Set disk manager task as READY
        disk_manager.status = TASK_READY;
        // Add disk manager task to the queue of ready tasks
        queue_append((queue_t **)&ready_tasks, (queue_t *)&disk_manager);
    }
    // Increase the semaphore count (unlock)
    sem_up(&disk.semaphore);
    // Suspend the current task, moving it from the ready_tasks queue to the disk queue
    task_suspend(&disk.queue);
    // Yield the CPU control to the dispatcher task
    task_yield();
    // Return success
    return 0;
}

int disk_block_write(int block, void *buffer)
{
    // Allocate memory for the request
    request_t *request = malloc(sizeof(request_t));
    // If memory allocation fails
    if (!request)
    {
        // Print an error message
        perror("ERROR: disk_block_write()=> Error on alocatting memory!\n");
        // Exit with error code
        exit(1);
    }
    // Set next and previous pointers as NULL
    request->next = request->prev = NULL;
    // Set task attribute to be the current task
    request->task = current_task;
    // Set the operation as WRITE
    request->operation = WRITE;
    // Set the block
    request->block = block;
    // Set the buffer
    request->buffer = buffer;
    // Decrease the semaphore count (lock)
    sem_down(&disk.semaphore);
    // Add the request to the queue of requests
    queue_append((queue_t **)&disk.requests, (queue_t *)request);
    // If disk manager task is SUSPENDED
    if (disk_manager.status == TASK_SUSPENDED)
    {
        // Set disk manager task as READY
        disk_manager.status = TASK_READY;
        // Add disk manager task to the queue of ready tasks
        queue_append((queue_t **)&ready_tasks, (queue_t *)&disk_manager);
    }
    // Increase the semaphore count (unlock)
    sem_up(&disk.semaphore);
    // Suspend the current task, moving it from the ready_tasks queue to the disk queue
    task_suspend(&disk.queue);
    // Yield the CPU control to the dispatcher task
    task_yield();
    // Return success
    return 0;
}

#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include "ppos.h"
#include "ppos_data.h"
#include "ppos_disk.h"
#include "disk.h"
#include "queue.h"

extern task_t *current_task;
extern task_t *ready_tasks;
int signal_disk;
struct sigaction action2;
disk_t disk;
task_t disk_manager;

void diskDriverBody()
{
    while (TRUE)
    {
        sem_down(&disk.semaphore);
        if (signal_disk)
        {
            task_t *task = disk.request->task;
            task->status = TASK_READY;
            task_resume(task, &disk.queue);
            queue_remove((queue_t **)&disk.requests, (queue_t *)disk.request);

            signal_disk = FALSE;
            free(disk.request);
        }
        int status_disk = disk_cmd(DISK_CMD_STATUS, 0, 0);
        if ((status_disk == DISK_STATUS_IDLE) && (disk.requests))
        {
            disk.request = disk.requests;
            if (disk.request->operation == WRITE)
            {
                disk_cmd(DISK_CMD_WRITE, disk.request->block, disk.request->buffer);
            }
            else if (disk.request->operation == READ)
            {
                disk_cmd(DISK_CMD_READ, disk.request->block, disk.request->buffer);                
            }
            else
            {
                perror("ERROR: diskDriverBody()=> Unknown request!\n");
                exit(1);
            }
        }
        sem_up(&disk.semaphore);
        disk_manager.status = TASK_SUSPENDED;
        queue_remove((queue_t **)&ready_tasks, (queue_t *)&disk_manager);
        task_yield();
    }
}

void handle_signal()
{
    signal_disk = TRUE;
    if (disk_manager.status == TASK_SUSPENDED)
    {
        disk_manager.status = TASK_READY;
        queue_append((queue_t **)&ready_tasks, (queue_t *)&disk_manager);
    }
}

int disk_mgr_init(int *numBlocks, int *blockSize)
{
    if (disk_cmd(DISK_CMD_INIT, 0, 0))
    {
        return -1;
    }
    *numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
    if (*numBlocks < 0)
    {
        return -1;
    }
    *blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
    if (*blockSize < 0)
    {
        return -1;
    }
    signal_disk = FALSE;
    sem_init(&disk.semaphore, 1);
    task_init(&disk_manager, diskDriverBody, NULL);
    disk_manager.status = TASK_SUSPENDED;
    // Sigaction configuration:
    action2.sa_handler = handle_signal;
    sigemptyset(&action2.sa_mask);
    action2.sa_flags = 0;
    if (sigaction(SIGUSR1, &action2, 0) < 0)
    {
        perror("ERROR: disk_mgr_init()=> Error on sigaction!\n");
        exit(1);
    }
    return 0;
}

int disk_block_read(int block, void *buf)
{
    request_type *req = malloc(sizeof(request_type));
    if (!req)
        return 0;
    req->task = current_task;
    req->operation = READ;
    req->buffer = buf;
    req->block = block;
    req->next = req->prev = NULL;
    sem_down(&disk.semaphore);
    queue_append((queue_t **)&disk.requests, (queue_t *)req);
    if (disk_manager.status == TASK_SUSPENDED)
    {
        disk_manager.status = TASK_READY;
        queue_append((queue_t **)&ready_tasks, (queue_t *)&disk_manager);
    }
    sem_up(&disk.semaphore);
    task_suspend(&disk.queue);
    task_yield();
    return 0;
}

int disk_block_write(int block, void *buf)
{
    request_type *req = malloc(sizeof(request_type));
    if (!req)
        return 0;
    req->task = current_task;
    req->operation = WRITE;
    req->buffer = buf;
    req->block = block;
    req->next = req->prev = NULL;
    sem_down(&disk.semaphore);
    queue_append((queue_t **)&disk.requests, (queue_t *)req);
    if (disk_manager.status == TASK_SUSPENDED)
    {
        disk_manager.status = TASK_READY;
        queue_append((queue_t **)&ready_tasks, (queue_t *)&disk_manager);
    }
    sem_up(&disk.semaphore);
    task_suspend(&disk.queue);
    task_yield();
    return 0;
}
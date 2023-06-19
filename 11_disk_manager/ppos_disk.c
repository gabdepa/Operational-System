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
disk_t disk;
task_t mngDiskTask;
int signal_disk;
struct sigaction action2;
void handle_signal();

static request_t *request(char reqType, void *buf, int block)
{
    request_t *req = malloc(sizeof(request_t));
    if (!req)
        return 0;
    req->req = current_task;
    req->type = reqType;
    req->buffer = buf;
    req->block = block;
    req->next = req->prev = NULL;
    return req;
}

void handle_signal()
{
    signal_disk = TRUE;
    if (mngDiskTask.status == TASK_SUSPENDED)
    {
        mngDiskTask.status = TASK_READY;
        queue_append((queue_t **)&ready_tasks, (queue_t *)&mngDiskTask);
    }
}

void diskDriverBody()
{
    while (TRUE)
    {
        sem_down(&disk.sem);
        if (signal_disk)
        {
            task_t *task = disk.request->req;
            task->status = TASK_READY;
            queue_remove((queue_t **)&disk.queue, (queue_t *)task);
            queue_append((queue_t **)&ready_tasks, (queue_t *)task);

            queue_remove((queue_t **)&disk.req_queue, (queue_t *)disk.request);

            signal_disk = FALSE;
            free(disk.request);
        }
        int status_disk = disk_cmd(DISK_CMD_STATUS, 0, 0);
        if ((status_disk == 1) && (disk.req_queue))
        {
            disk.request = disk.req_queue;
            if (disk.request->type == 'R')
                disk_cmd(DISK_CMD_READ, disk.request->block, disk.request->buffer);
            else
                disk_cmd(DISK_CMD_WRITE, disk.request->block, disk.request->buffer);
        }
        sem_up(&disk.sem);
        mngDiskTask.status = TASK_SUSPENDED;
        queue_remove((queue_t **)&ready_tasks, (queue_t *)&mngDiskTask);
        task_yield();
    }
}

int disk_mgr_init(int *numBlocks, int *blockSize)
{

    if (disk_cmd(DISK_CMD_INIT, 0, 0) || ((*numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0)) < 0) || ((*blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0)) < 0))
        return -1;

    signal_disk = FALSE;
    sem_init(&disk.sem, 1);
    task_init(&mngDiskTask, diskDriverBody, NULL);
    mngDiskTask.status = TASK_SUSPENDED;
    queue_remove((queue_t **)&ready_tasks, (queue_t *)&mngDiskTask);

    action2.sa_handler = handle_signal;
    sigemptyset(&action2.sa_mask);
    action2.sa_flags = 0;
    if (sigaction(SIGUSR1, &action2, 0) < 0)
    {
        perror("Erro em sigaction: ");
        exit(1);
    }

    return 0;
}

int disk_block_read(int block, void *buf)
{
    sem_down(&disk.sem);

    request_t *req = request('R', buf, block);
    if (!req)
        return -1;
    queue_append((queue_t **)&disk.req_queue, (queue_t *)req);
    if (mngDiskTask.status == TASK_SUSPENDED)
    {
        mngDiskTask.status = TASK_READY;
        queue_append((queue_t **)&ready_tasks, (queue_t *)&mngDiskTask);
    }

    sem_up(&disk.sem);
    current_task->status = TASK_SUSPENDED;
    queue_remove((queue_t **)&ready_tasks, (queue_t *)current_task);
    queue_append((queue_t **)&disk.queue, (queue_t *)current_task);

    task_yield();
    return 0;
}

int disk_block_write(int block, void *buf)
{
    sem_down(&disk.sem);

    request_t *newRequest = request('W', buf, block);
    if (!newRequest)
        return -1;
    queue_append((queue_t **)&disk.req_queue, (queue_t *)newRequest);
    if (mngDiskTask.status == TASK_SUSPENDED)
    {
        mngDiskTask.status = TASK_READY;
        queue_append((queue_t **)&ready_tasks, (queue_t *)&mngDiskTask);
    }

    sem_up(&disk.sem);
    current_task->status = TASK_SUSPENDED;
    queue_remove((queue_t **)&ready_tasks, (queue_t *)current_task);
    queue_append((queue_t **)&disk.queue, (queue_t *)current_task);

    task_yield();
    return 0;
}
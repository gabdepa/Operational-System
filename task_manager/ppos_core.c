// GRR20197155 Gabriel Razzolini Pires De Paula

// Including libs needed
#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

queue_t *ready_queue = NULL; // Queue of ready tasks
task_t main_task;            // Main Task
task_t *current_task;        // Current running Task
int last_id;                 // Last Task ID

// Initialize the OS, must be called at the beginning of main()
void ppos_init()
{
    // Disable stdout buffering to ensure that printf output appears immediately
    setbuf(stdout, NULL);
    // Initialize the task ID counter
    last_id = 0;
    // Set up the main task
    task_t *main_task_ptr = &main_task;
    main_task_ptr->id = last_id;
    // Assign the main task as the current task
    current_task = main_task_ptr;
    // Obtain the current execution context and store it in the main task's context
    ucontext_t *main_task_context = &(main_task_ptr->context);
    if (getcontext(main_task_context) == -1)
    {
        // If getcontext fails (returns -1), print an error message
        perror("ERROR: Failed to get context for main task");
        // Terminate the program with an error status (1)
        exit(1);
    }
}

// Initialize a new Task. Returns <ID> or ERROR CODE
int task_init(task_t *task, void (*start_func)(void *), void *arg)
{
    // Initialize the task's context
    ucontext_t *task_context = &(task->context);
    if (getcontext(task_context) == -1)
    {
        perror("ERROR: Failed to get context for the new task!");
        exit(1);
    }
    // Allocate the task's stack
    void *stack = malloc(STACKSIZE);
    // If could not allocate
    if (!stack)
    {
        // Print error message
        perror("ERROR on stack creation! ");
        // Exit with error code
        exit(1);
    }
    // Assign the allocated stack memory to the task's context stack pointer
    task_context->uc_stack.ss_sp = stack;
    // Set the size of the task's context stack to the predefined STACKSIZE
    task_context->uc_stack.ss_size = STACKSIZE;
    // Initialize the task's context stack flags to 0 (no flags)
    task_context->uc_stack.ss_flags = 0;
    // Set the task's context link (uc_link) to 0, meaning there is no successor context when this context is finished executing
    task_context->uc_link = 0;
    // Set up the task's context to execute 'start_func' with the argument 'arg'
    makecontext(task_context, (void (*)(void))start_func, 1, arg);
    // Assign a unique ID to the task and update the last_id
    task->id = ++last_id;
    task->status = TASK_READY;
#ifdef DEBUG
    printf("task_init()=> Initialized Task  %d\n", task->id);
#endif
    // Return the ID of the initialized task
    return task->id;
}

// Return ID of the Current Task
int task_id()
{
    return current_task->id;
}

// Change execution from the current task to the specified task "task"
int task_switch(task_t *task)
{
    // Check if the given task is NULL, and if so, return an error code (-1)
    if (!task)
    {
        // Print error message
        perror("ERROR: task specified does not exist!");
        // Exit with error code
        exit(1);
    }
    // Store the previous task before making the switch
    task_t *previous_task = current_task;
    // Update the current task pointer to point to the new task
    current_task = task;
#ifdef DEBUG
    printf("task_switch()=> Switched context %d to %d\n", previous_task->id, current_task->id);
#endif
    // Save the current execution context into the previous task's context
    if (getcontext(&(previous_task->context)) == 0)
    {
        // If the current task is not the previous task, perform the context switch
        if (current_task != previous_task)
        {
            // Update the status of the previous task to TASK_READY
            previous_task->status = TASK_READY;
            // Update the status of the current (new) task to TASK_RUNNING
            current_task->status = TASK_RUNNING;
            // Restore the execution context of the current (new) task
            setcontext(&(current_task->context));
        }
    }
    // Return success (0)
    return 0;
}

// Terminate the current task and return control to the main task with a given exit code
void task_exit(int exit_code)
{
    // Set the status of the current task to TASK_TERMINATED
    current_task->status = TASK_TERMINATED;
#ifdef DEBUG
    printf("task_exit()=> Task %d terminated with exit code %d\n", current_task->id, exit_code);
#endif
    // Check if the current task is the main task
    if (current_task == &main_task)
    {
        // If it is, simply return to avoid switching to the main task again
        return;
    }
    else // If the current task is not the main task
    {
        // Transfer control back to the main task
        task_switch(&main_task);
    }
}

// Suspend the current task and move it from the ready queue to the specified queue
void task_suspend(task_t **queue)
{
    // Check if the queue pointer is valid
    if (!queue)
    {
        // Print error message
        perror("ERROR: Invalid queue!");
        // Exit with error code
        exit(1);
    }
    // Check if the current_task pointer is valid
    else if (!current_task)
    {
        // Print error message
        perror("ERROR: Invalid current_task!");
        // Exit with error code
        exit(1);
    }
    else
    {
        // Save the next task in the ready queue
        task_t *next_ready_task = (task_t *)ready_queue;
        // If there is a task in the ready queue, update the ready queue
        if (next_ready_task)
        {
            ready_queue = ready_queue->next;
        }
        // Remove the current task from the ready queue
        int removal_result = queue_remove(&ready_queue, (queue_t *)current_task);
        // If the removal is successful, update the current task and move it to the specified queue
        if (removal_result == 0)
        {
            current_task->status = TASK_SUSPENDED;
            queue_append((queue_t **)queue, (queue_t *)current_task);
        }
        // If there is a next task in the ready queue, switch to it
        if (next_ready_task)
        {
            task_switch(next_ready_task);
        }
    }
}

// Resume the Task "task", moving it from the "queue" queue to the "ready" queue.
void task_resume(task_t *task, task_t **queue)
{
    /*************************ERROR CASES**********************/
    // If the "task" pointer is NULL
    if (!task)
    {
        // Print error message
        perror("ERROR: task specified does not exist!");
        // Exit
        exit(1);
    }
    // If the "queue" pointer is NULL
    else if (!queue)
    {
        // Print error message
        perror("ERROR: queue specified does not exist!");
        // Exit
        exit(1);
    } /*************************ERROR CASES**********************/
    else
    {
        // If the removal of task from queue was successful
        if (queue_remove((queue_t **)queue, (queue_t *)task) == 0)
        {
            // Append the task to the "ready" queue
            queue_append(&ready_queue, (queue_t *)task);
            // Set the task status to TASK_READY
            task->status = TASK_READY;
        }
        else
        {
            // Print error message
            printf("ERROR: Could not remove task %d from queue!\n", task->id);
            // Exit with error code
            exit(1);
        }
    }
}
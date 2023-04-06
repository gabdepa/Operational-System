// GRR20197155 Gabriel Razzolini Pires De Paula

// Including libs needed
#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

// Size of threads stack
#define STACKSIZE 64 * 1024

// Constants status value
#define TASK_READY 0
#define TASK_RUNNING 1
#define TASK_SUSPENDED 2

queue_t *ready_queue = NULL; // Queue of ready tasks
task_t main_task;     // Main Task
task_t *current_task; // Current running Task
int last_id;          // Last Task ID

// Initialize the OS, must be call in the begin of main()
void ppos_init()
{
    // Deactivate the buffer of stdout, used by "printf"
    setvbuf(stdout, 0, _IONBF, 0);
    // Initialize last ID to keep track of the last task ID
    last_id = 0;
    // Initialize Main Task ID
    main_task.id = 0;
    // Set Current task to track Main Task
    current_task = &main_task;
    // Save the current context into "context" attribute of Main Task
    getcontext( &(main_task.context) );
}

// Initialize a new Task. Returns <ID> or ERROR CODE
int task_init(task_t *task, void (*start_func)(void *), void *arg)
{
    // Save the current context into "context" attribute of "task" Task
    getcontext( &(task->context) );
    // Allocate memory for the stack pointer
    char *stack = malloc(STACKSIZE);
    // If it was successfully allocated
    if( stack )
    {
        // Change the Stack pointer attribute to "stack"
        task->context.uc_stack.ss_sp = stack;
        // Change the Stack size attribute to "STACKSIZE"
        task->context.uc_stack.ss_size = STACKSIZE;
        // Change Stack Flags attribute to 0
        task->context.uc_stack.ss_flags = 0;
        // Change Link to 0
        task->context.uc_link = 0;
    }
    else // If "stack" was not successfully allocated
    {
        // Write error message
        perror("ERROR on stack creation-> ");
        // Exit with error code
        exit(1);
    }
    // Set up the task's context to execute the 'start_func' function with the argument 'arg' when the task is scheduled to run
    makecontext( &(task->context), (void *)(*start_func), 1, arg );
    // "task" attribute ID receives the next ID, comparing to the last one
    task->id = ++last_id;

#ifdef DEBUG
printf ("task_init()=> Initialized Task  %d\n", task->id);
#endif

    // Set the task status to TASK_READY
    task->status = TASK_READY;

    // Return the ID of "task"
    return task->id;
}

// Return ID of the Current Task
int task_id()
{
    return current_task->id;
}

// Switch execution to the indicated Task "task"
int task_switch(task_t *task)
{
    // Saving Current Task to Last task
    task_t *last_task = current_task;
    // Updating Current Task to the indicated Task -> "task"
    current_task = task;
    // Change of context to Current Task, which now points to &task
    swapcontext( &(last_task->context), &(current_task->context) );

#ifdef DEBUG
printf ("task_switch()=> Swithced context %d to %d\n", last_task->id, current_task->id);
#endif

    // Set the status of the task being switched to as TASK_RUNNING
    task->status = TASK_RUNNING;
    // Set the status of the task being switched out as TASK_READY
    last_task->status = TASK_READY;
    // Return Success
    return 0;
}

// Ends Current Task with a status value of termination
void task_exit(int exit_code)
{

#ifdef DEBUG
printf ("task_exit()=> Task %d being terminated\n", main_task.id);
#endif

    // Switch execution to the Main Task
    task_switch( &main_task );
}

// Suspend the Current Task, moving it from the "ready" queue to the specified queue, "queue"
void task_suspend(task_t **queue)
{
    /*************************ERROR CASES**********************/
    // If the "queue" pointer is NULL
    if( !queue )
    {
        // Print error message
        perror("ERROR: queue specified does not exist!");
        // Exit
        exit(1);
    }
    // If the "current_task" pointer is NULL
    if( !current_task )
    {
        // Print error message
        perror("ERROR: current_task does not exists!");
        // Exit
        exit(1);
    }
    /*************************ERROR CASES**********************/

    // Get the next task from the "ready" queue
    task_t *next_task = (task_t *)ready_queue;

    // If there is a next task in the "ready" queue
    if( next_task )
    {
        // Move to the next task in the "ready" queue
        ready_queue = ready_queue->next;
    }

    // Remove the current task from the "ready" queue
    int remove_result = queue_remove( &ready_queue, (queue_t *)current_task );

    // If the removal was successful
    if( remove_result == 0 )
    {
        // Set the current task status to TASK_SUSPENDED
        current_task->status = TASK_SUSPENDED;

        // Append the current task to the specified "queue"
        queue_append( (queue_t **)queue, (queue_t *)current_task );
    }

    // If there is a next task in the "ready" queue
    if( next_task )
    {
        // Switch to the next task
        task_switch( next_task );
    }
}

// Resume the Task "task", moving it from the "queue" queue to the "ready" queue.
void task_resume (task_t *task, task_t **queue)
{
    /*************************ERROR CASES**********************/
    // If the "task" pointer is NULL
    if( !task  )
    {
        // Print error message
        perror("ERROR: task specified does not exist!");
        // Exit
        exit(1);
    }
    // If the "queue" pointer is NULL
    if( !queue )
    {
        // Print error message
        perror("ERROR: queue specified does not exist!");
        // Exit
        exit(1);
    }
    /*************************ERROR CASES**********************/

    // Remove the task from the queue "queue"
    int remove_result = queue_remove( (queue_t **)queue, (queue_t *)task );

    // If the removal was successful 
    if( remove_result == 0 )
    {
        // Set the task status to TASK_READY
        task->status = TASK_READY;

        // Append the task to the "ready" queue
        queue_append( &ready_queue, (queue_t *)task );
    }
}
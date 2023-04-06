// GRR20197155 Gabriel Razzolini Pires De Paula

// Including libs needed
#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

// Debug printer
#ifdef DEBUG
#define debug_print(...) \
  do                     \
  {                      \
    printf(__VA_ARGS__); \
  } while (0)
#else
#define debug_print(...) \
  do                     \
  {                      \
  } while (0)
#endif

queue_t *ready_queue = NULL; // Queue of TASK_READY tasks: This queue stores tasks that are ready to be executed.
task_t main_task;            // Main Task: This variable represents the main task of the program.
task_t *current_task;        // Current running Task: This pointer points to the task that is currently running.
task_t *tasks_queue;         // Queue of tasks: This pointer represents the queue of all tasks, which includes tasks with different states (e.g., TASK_READY, TASK_RUNNING, TASK_TERMINATED).
task_t dispatcher;           // Dispatcher: This variable represents the dispatcher task, which is responsible for managing tasks execution and switching between them.

int last_id;    // Last Task ID: This variable keeps track of the last assigned task ID. It is used to generate unique IDs for new tasks.
int user_tasks; // Current quantity tasks of the user: This variable maintains a count of the current number of user tasks (excluding system tasks like the dispatcher). It is helpful for managing and monitoring the overall state of the system.

// The scheduler function returns the next task to be executed, using the First-Come, First-Served (FCFS) scheduling policy.
task_t *scheduler()
{
    // Returns the head of the tasks_queue, which is the next task to be executed according to the FCFS policy.
    return tasks_queue;
}

// Used in the debug rule, the function takes a pointer to a task structure, casts it to a task_t pointer, and prints the task ID.
void print_element(void *ptr)
{
    // Cast the void pointer to a task_t pointer
    task_t *tmp_task = ptr;
    // Check if the task pointer is NULL
    if ( !tmp_task )
    {
        // If it is, return immediately, ending execution
        return;
    }
    // Print the task ID, which is an integer
    printf("%d", tmp_task->id);
}

// The dispatcher_body function is responsible for managing the execution of user tasks.
void dispatcher_body()
{
    // The dispatcher loop continues running until there are no more user tasks.
    while ( user_tasks > 0 )
    {
#ifdef DEBUG
        // / Print a debug message indicating elements id in the queue of tasks
        queue_print("PPOS: tasks_queue ", (queue_t *)tasks_queue, print_element);
#endif
        // Get the next task to be executed using the scheduler function.
        task_t *next_task = scheduler();
        // If there is a valid next task to be executed.
        if ( next_task )
        {
            // Perform a context switch to the next task.
            task_switch( next_task );
            // Depending on the status of the next task after execution, take appropriate action.
            switch ( next_task->status )
            {
            // If the task is in the TASK_READY state, move it to the end of the queue.
            case TASK_READY:
                tasks_queue = tasks_queue->next;
                break;
            // If the task is in the TASK_TERMINATED state, remove it from the queue.
            case TASK_TERMINATED:
                queue_remove( (queue_t **)&tasks_queue, (queue_t *)next_task );
                break;
            }
        }
    }

    // When there are no more user tasks, switch back to the main task.
    task_switch( &main_task );
}

// This function initializes the PingPongOS. It must be called at the beginning of the main function.
void ppos_init()
{
    // Deactivate the buffer of stdout, used by "printf" to make sure that the printed text appears immediately.
    setvbuf(stdout, 0, _IONBF, 0);
    // Initialize the last ID variable to keep track of the last task ID.
    last_id = 0;
    // Set the ID of the main task.
    main_task.id = 0;
    // Set the state of the main task to ready.
    main_task.status = TASK_READY;
    // Set the current task to the main task.
    current_task = &main_task;
    // Save the current context into the "context" attribute of the main task.
    getcontext( &(main_task.context) );
    // Get the context of the main task again (redundant line, can be removed).
    getcontext( &(main_task.context) );
    // Set the status of the main task to ready (redundant line, can be removed).
    main_task.status = TASK_READY;
    // Create the dispatcher task.
    task_init( &dispatcher, dispatcher_body, NULL );
    // Remove the dispatcher task from the tasks_queue, as it should not be managed by the scheduler.
    queue_remove( (queue_t **)&tasks_queue, (queue_t *)&dispatcher );
    // Initialize the user_tasks counter to 0.
    user_tasks = 0;
    // Print a debug message indicating that the system is starting.
    debug_print("PPOS: Starting the system...\n");
}

// Initialize a new Task. Returns <ID> or ERROR CODE
int task_init(task_t *task, void (*start_func)(void *), void *arg)
{
    // Set the task status to new task received
    task->status = NEW_TASK;
    // Save the current context into "context" attribute of "task" Task
    getcontext(&(task->context));
    // Allocate memory for the stack pointer
    char *stack = malloc(STACKSIZE);
    // If it was successfully allocated
    if ( stack )
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
    // Set the task status to TASK_READY
    task->status = TASK_READY;
    // "task" attribute ID receives the next ID, comparing to the last one
    task->id = ++last_id;
    // Append the task to the queue of tasks
    queue_append( (queue_t **)&tasks_queue, (queue_t *)task );
    // Update number of user tasks
    user_tasks++;
    // Print for debbuging
    debug_print("PPOS: Task created with id %d, currently there are %d user tasks\n", last_id - 1, user_tasks);
    // Return the ID of "task"
    return task->id;
}

// Return ID of the Current Task
int task_id()
{
    return current_task->id;
}

// This function voluntarily gives up the processor, allowing other tasks to run.
void task_yield()
{
    // Switch the execution to the dispatcher task, which will handle task scheduling.
    task_switch( &dispatcher );
}

// Switches execution to the indicated task "task"
int task_switch(task_t *task)
{
    // Save the current task as the last task
    task_t *last_task = current_task;
    // Update the current task to the indicated task -> "task"
    current_task = task;
    // Set the last task's status to TASK_TERMINATED if it was terminated, otherwise set it to TASK_READY
    last_task->status = last_task->status == TASK_TERMINATED ? TASK_TERMINATED : TASK_READY;
    // Set the current task's status to TASK_RUNNING
    current_task->status = TASK_RUNNING;
    // Print a debug message showing the task switch
    debug_print("PPOS: Switching tasks from: %d to: %d\n", last_task->id, current_task->id);
    // Swap the context from the last task to the current task
    swapcontext( &(last_task->context), &(current_task->context) );
    // Return success (0)
    return 0;
}

// Ends the current task with a specified exit code
void task_exit(int exit_code)
{
    // Set the current task's status to TASK_TERMINATED
    current_task->status = TASK_TERMINATED;
    // Decrement the number of user tasks
    user_tasks--;
    // Print a debug message showing the task termination
    debug_print("PPOS: Terminating task with id %d\n", current_task->id);
#ifdef DEBUG
    // Print a debug message indicating that the task is being terminated
    debug_print("task_exit() => Task %d being terminated\n", main_task.id);
#endif
    // Switch execution to the dispatcher
    task_switch( &dispatcher );
}

// Suspend the Current Task, moving it from the "ready" queue to the specified queue, "queue"
void task_suspend(task_t **queue)
{
    /*************************ERROR CASES**********************/
    // If the "queue" pointer is NULL
    if ( !queue )
    {
        // Print error message
        perror("ERROR: queue specified does not exist!");
        // Exit
        exit(1);
    }
    // If the "current_task" pointer is NULL
    if ( !current_task )
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
    if ( next_task )
    {
        // Move to the next task in the "ready" queue
        ready_queue = ready_queue->next;
    }
    // Remove the current task from the "ready" queue
    int remove_result = queue_remove( &ready_queue, (queue_t *)current_task );
    // If the removal was successful
    if ( remove_result == 0 )
    {
        // Set the current task status to TASK_SUSPENDED
        current_task->status = TASK_SUSPENDED;

        // Append the current task to the specified "queue"
        queue_append( (queue_t **)queue, (queue_t *)current_task );
    }

    // If there is a next task in the "ready" queue
    if ( next_task )
    {
        // Switch to the next task
        task_switch( next_task );
    }
}

// Resume the Task "task", moving it from the "queue" queue to the "ready" queue.
void task_resume(task_t *task, task_t **queue)
{
    /*************************ERROR CASES**********************/
    // If the "task" pointer is NULL
    if ( !task )
    {
        // Print error message
        perror("ERROR: task specified does not exist!");
        // Exit
        exit(1);
    }
    // If the "queue" pointer is NULL
    if ( !queue )
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
    if ( remove_result == 0 )
    {
        // Set the task status to TASK_READY
        task->status = TASK_READY;
        // Append the task to the "ready" queue
        queue_append(&ready_queue, (queue_t *)task);
    }
}
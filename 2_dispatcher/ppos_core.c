// GRR20197155 Gabriel Razzolini Pires De Paula

// Including libs needed
#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"
#include "ppos_data.h"
#include "queue.h"

// Debug printer
#ifdef DEBUG
#define debug_print(...)     \
    do                       \
    {                        \
        printf(__VA_ARGS__); \
    } while (0)
#else
#define debug_print(...) \
    do                   \
    {                    \
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
    // Check if the pointer is NULL
    if (ptr != NULL)
    {
        // Cast the void pointer to a task_t pointer and dereference it to access the task ID
        int task_id = ((task_t *)ptr)->id;
        // Print the task ID, which is an integer
        printf("%d", task_id);
    }
}

// The dispatcher_body function is responsible for managing the execution of user tasks.
void dispatcher_body()
{
    task_t *next_task;
    // The dispatcher loop continues running until there are no more user tasks.
    while (user_tasks > 0 && tasks_queue != NULL)
    {
        // Get the next task to be executed using the scheduler function.
        next_task = scheduler();
        // Check if a valid next task is found and its status is not TASK_TERMINATED
        if (next_task != NULL && next_task->status != TASK_TERMINATED)
        {
            // Perform a context switch to the next task.
            task_switch(next_task);

            // Handle the status of the next task after execution
            if (next_task->status == TASK_READY)
            {
                // If the task is in the TASK_READY state, move it to the end of the queue.
                tasks_queue = tasks_queue->next;
            }
            else if (next_task->status == TASK_TERMINATED) // If the task is in the TASK_TERMINATED state remove it from the queue.
            {
                if ( !queue_remove((queue_t **)&tasks_queue, (queue_t *)next_task) ) // If could not remove from the queue
                {
                    // Print a debug message 
                    debug_print("PPOS: Failed to remove terminated task %d from tasks_queue\n", next_task->id);
                }
            }
        }
#ifdef DEBUG
        // Print a debug message indicating elements id in the queue of tasks
        queue_print("PPOS: tasks_queue ", (queue_t *)tasks_queue, print_element);
        debug_print("PPOS: user_tasks: %d, next_task_id: %d\n", user_tasks, next_task->id);
#endif
    }
    // When there are no more user tasks, switch back to the main task.
    task_switch(&main_task);
}

// This function initializes the PingPongOS. It must be called at the beginning of the main function.
void ppos_init()
{
    // Disable buffering for stdout so that printed text appears immediately.
    setbuf(stdout, NULL);
    // Initialize global variables for task management.
    last_id = 0;    // Last Task ID
    user_tasks = 0; // Current number of user tasks
    // Set up the main task.
    main_task.id = 0;              // ID of the main task
    main_task.status = TASK_READY; // Main task is ready
    current_task = &main_task;     // Set the current task to the main task
    // Save the current context into the "context" attribute of the main task.
    getcontext(&(main_task.context));
    // Initialize the dispatcher task.
    task_init(&dispatcher, dispatcher_body, NULL);
    // Remove the dispatcher task from the tasks_queue because it's managed separately.
    queue_remove((queue_t **)&tasks_queue, (queue_t *)&dispatcher);
    // Print a debug message indicating that the system is starting.
    debug_print("PPOS: Starting the system...\n");
}

// Initialize a new Task. Returns <ID> or ERROR CODE
int task_init(task_t *task, void (*start_func)(void *), void *arg)
{
    // Set the initial status for the task
    task->status = NEW_TASK;
    // Get the current context for the task
    if (getcontext(&(task->context)) == -1)
    {
        perror("ERROR: Failed to get context");
        exit(1);
    }
    // Allocate memory for the task's stack and set the stack attributes
    void *stack = malloc(STACKSIZE);
    if (stack == NULL)
    {
        perror("ERROR: Stack allocation failed");
        exit(1);
    }
    // Assign the allocated stack memory to the task's context stack pointer
    task->context.uc_stack.ss_sp = stack;
    // Set the task's context stack size to the pre-defined STACKSIZE constant
    task->context.uc_stack.ss_size = STACKSIZE;
    // Set the task's context stack flags to 0 (no special flags)
    task->context.uc_stack.ss_flags = 0;
    // Set the task's context link to 0, which means no specific context to return to after task's completion
    task->context.uc_link = 0;
    // Configure the task's context to execute 'start_func' with 'arg' when scheduled
    makecontext(&(task->context), (void (*)(void))start_func, 1, arg);
    // Update the task's status, ID, and add it to the tasks_queue
    task->status = TASK_READY;
    task->id = ++last_id;
    queue_append((queue_t **)&tasks_queue, (queue_t *)task);
    // Increment the user_tasks counter
    user_tasks++;
    // Debug message
    debug_print("PPOS: Task created with id %d, currently there are %d user tasks\n", last_id - 1, user_tasks);
    // Return the task's ID
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
    task_switch(&dispatcher);
}

// Switches execution to the indicated task "task"
int task_switch(task_t *task)
{
    // If the given task is NULL, return an error code (-1)
    if (!task)
    {
        return -1;
    }
    // Store the previous task before switching
    task_t *previous_task = current_task;
    // Update the current task pointer to the new task
    current_task = task;
    // Update the status of the previous task based on its previous status
    if (previous_task->status != TASK_TERMINATED)
    {
        previous_task->status = TASK_READY;
    }
    // Set the new task's status to TASK_RUNNING
    current_task->status = TASK_RUNNING;
    // Print a debug message to indicate the task switch
    debug_print("PPOS: Switching tasks from: %d to: %d\n", previous_task->id, current_task->id);
    // Perform the context switch between the previous task and the new task
    int result = swapcontext(&(previous_task->context), &(current_task->context));
    // If the context switch is successful, return 0; otherwise, return -1
    return result == -1 ? -1 : 0;
}

// Ends the current task with a specified exit code
void task_exit(int exit_code)
{
    // Change the status of the currently running task to TASK_TERMINATED
    current_task->status = TASK_TERMINATED;
#ifdef DEBUG
    // Print an additional debug message to show that the task is being terminated
    debug_print("task_exit() => Terminating Task %d\n", current_task->id);
#endif
    // Log the task termination for debugging purposes
    debug_print("PPOS: Task %d terminated with exit code %d\n", current_task->id, exit_code);
    // Update the user tasks count by decrementing it by one
    --user_tasks;
    // Transfer control to the dispatcher
    task_switch(&dispatcher);
}

// Suspend the Current Task, moving it from the "ready" queue to the specified queue, "queue"
void task_suspend(task_t **queue)
{
    // Validate that the "queue" and "current_task" pointers are not NULL
    if (!queue || !current_task)
    {
        // Print error message and exit
        perror("ERROR: Invalid queue or current task!");
        exit(1);
    }
    // Obtain the next task from the "ready" queue
    task_t *next_task_in_queue = (task_t *)ready_queue;
    // Update the "ready" queue, moving to the next task
    if (next_task_in_queue)
    {
        ready_queue = ready_queue->next;
    }
    // Remove the current task from the "ready" queue and check if successful
    if (queue_remove(&ready_queue, (queue_t *)current_task) == 0)
    {
        // Set the current task status to TASK_SUSPENDED
        current_task->status = TASK_SUSPENDED;
        // Add the current task to the specified "queue"
        queue_append((queue_t **)queue, (queue_t *)current_task);
    }
    // If there is a next task in the "ready" queue, switch to it
    if (next_task_in_queue)
    {
        task_switch(next_task_in_queue);
    }
}

// Resume the Task "task", moving it from the "queue" queue to the "ready" queue.
void task_resume(task_t *task, task_t **queue)
{
    // Validate that the "task" and "queue" pointers are not NULL
    if (!task || !queue)
    {
        // Print error message and exit
        perror("ERROR: Invalid task or queue!");
        exit(1);
    }
    // Attempt to remove the task from the specified "queue"
    int removal_successful = queue_remove((queue_t **)queue, (queue_t *)task);
    // If removal was successful, update the task status and add it to the "ready" queue
    if (removal_successful == 0)
    {
        // Update the task status to TASK_READY
        task->status = TASK_READY;
        // Add the task to the "ready" queue
        queue_append(&ready_queue, (queue_t *)task);
    }
}

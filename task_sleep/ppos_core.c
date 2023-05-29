// GRR20197155 Gabriel Razzolini Pires De Paula

// Including libs needed
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
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

task_t main_task;              // Main Task: This variable represents the main task of the program.
task_t *current_task;          // Current running Task: This pointer points to the task that is currently running.
task_t *tasks_queue;           // Queue of tasks: This pointer represents the queue of all tasks, which includes tasks with different states (e.g., TASK_READY, TASK_RUNNING, TASK_TERMINATED).
task_t *suspended_tasks_queue; // Queue of Suspended Tasks: This pointer represents the queue of tasks that are in state "TASK_SUSPENDED" and are putting to sleep.
task_t dispatcher;             // Dispatcher: This variable represents the dispatcher task, which is responsible for managing tasks execution and switching between them.

int last_id;              // Last Task ID: This variable keeps track of the last assigned task ID. It is used to generate unique IDs for new tasks.
int user_tasks;           // Current quantity tasks of the user: This variable maintains a count of the current number of user tasks (excluding system tasks like the dispatcher). It is helpful for managing and monitoring the overall state of the system.
int task_timer;           // Task Timer: This variable keeps track of the quantum of the task running
unsigned int system_time; // System Time: Global variable that tells the time the system is running

// Aditional structs needed to use preemption
struct sigaction action; // Struct Action used
struct itimerval timer;  // Struct Timer used

// Function to print the details of a task element
void print_element(void *ptr)
{
    // Cast the input void pointer to a task_t pointer
    task_t *task = ptr;
    // If the task pointer is NULL, exit the function
    if (!task)
        return;
    // Print the task ID and dynamic priority of the task
    printf("Task ID->%d|dynamicPriority->(%d)|staticPriority->(%d),  ", task->id, task->dynamicPriority, task->staticPriority);
}

// Determines the next task to be executed
task_t *scheduler()
{
    // Set tmp_task to the next task in the queue
    task_t *tmp_task = tasks_queue->next;
    // Set lowest_priority_task to the first task in the queue
    task_t *lowest_priority_task = tasks_queue;
    // Set queue_start to the next task in the queue (same as tmp_task initially)
    task_t *queue_start = tasks_queue->next;
    // Loop through the tasks queue
    do
    {
#ifdef DEBUG
        debug_print("PPOS: scheduler()=> Current in task: %d of the queue of tasks.\n", tmp_task->id);
#endif
        // Check if the current task has a lower dynamic priority than the current lowest_priority_task
        if (tmp_task->dynamicPriority < lowest_priority_task->dynamicPriority)
        {
#ifdef DEBUG
            debug_print("PPOS: scheduler()=> Found task with the lowest dynamic priority, task: %d\n", tmp_task->id);
#endif
            // Set the new lowest_priority_task
            lowest_priority_task = tmp_task;
        }
        // Check if the current task is not the lowest priority task
        if (tmp_task != lowest_priority_task)
        {
#ifdef DEBUG
            debug_print("PPOS: scheduler()=> Lowing the dynamic priority of task: %d\n", tmp_task->id);
#endif
            // If dynamic priority is at the lowest
            if (tmp_task->dynamicPriority <= -20)
            {
                // Reset to the lowest dynamic priority
                tmp_task->dynamicPriority = -20;
            }
            else // If it's not at the lowest
            {
                // Lower the dynamic priority of the current task
                tmp_task->dynamicPriority--;
            }
        }
        // Move on to the next task in the queue
        tmp_task = tmp_task->next;
        // Continue looping until we reach the initial starting point in the queue
    } while (tmp_task != queue_start);
    // Return the task with the lowest dynamic priority
    return lowest_priority_task;
}

// Returns current time of the system
unsigned int systime()
{
    return system_time;
}

// Config auxiliar function that reset some properties of the task
void task_reset(task_t *task)
{
    // Reset the task timer to its initial value
    task_timer = TASK_TIMER;
    // If the dynamic priority diverges from the static
    if (task->staticPriority != task->dynamicPriority)
    {
        // Reset the dynamic back to be equal as the static
        task->dynamicPriority = task->staticPriority;
    }
}

void task_wakeup()
{
    // Queue of suspended tasks is empty
    if (suspended_tasks_queue == NULL)
    {
        return;
    }
    // Initialize a pointer to the first suspended task in queue
    task_t *aux = suspended_tasks_queue;
    // Save the pointer to the first task as the starting point for the loop
    task_t *start = aux;
    // Declare a pointer to store the next task in the queue
    task_t *next_aux;
    // Start a loop that will go through each suspended task in queue
    do
    {
        // Store the next task before resuming the current one
        next_aux = aux->next;
        if (systime() >= suspended_tasks_queue)
        {
            // Call the resume function for the current task
            task_resume(aux, &suspended_tasks_queue);
        }
        // Move to the next task in the suspended tasks queue
        aux = next_aux;
        // Continue looping as long as the current task is not the head of the queue
    } while (aux != start);
}

// The dispatcher_body function is responsible for managing the execution of user tasks.
void dispatcher_body()
{
    // Set the status of the dispatcher task
    dispatcher.status = TASK_RUNNING;
    // Increase the amount of activations of the dispatcher
    dispatcher.activations++;
    // Create new variable to represent the next task
    task_t *next_task;
    // The dispatcher loop continues running until there are no more user tasks.
    while (user_tasks > 0)
    {
        // Get the next task to be executed using the scheduler function.
        next_task = scheduler();
        // Reconfig the next_task
        task_reset(next_task);
        // Verify if status of the next task is NOT in TASK_TERMINATED
        if (next_task->status != TASK_TERMINATED)
        {
            // Increase the amount of activations of the task
            next_task->activations++;
#ifdef DEBUG
            debug_print("PPOS: dispatcher_body()=> Dispatcher was activated %d times.\n", dispatcher.activations);
            debug_print("PPOS: dispatcher_body()=> Number of user tasks: %d, next task id: %d, %d  activations.\n", user_tasks, next_task->id, next_task->activations);
#endif
            // Perform a context switch to the next task.
            task_switch(next_task);
            // Add processing time to the execution time
            next_task->executionTime += next_task->processingTime;
            // Increase the amount of activations of the dispatcher
            dispatcher.activations++;
            // Handle the status of the next task after execution
            switch (next_task->status)
            {
            case TASK_READY:
                break;
            case TASK_SUSPENDED:
                break;
            }
        }
#ifdef DEBUG
        queue_print("PPOS: dispatcher_body()=> Queue of tasks: ", (queue_t *)tasks_queue, print_element);
#endif
    }
    // Set the status of the dispatcher task
    dispatcher.status = TASK_SUSPENDED;
    // If there are no more user tasks to execute and the current task running is the dispatcher
    if ((user_tasks == 0) && (current_task->id == dispatcher.id))
    {
        // Update the id of the last task to be the dispatcher id
        last_id = current_task->id;
        // Change the status of the currently running task(dispatcher) to TASK_TERMINATED
        current_task->status = TASK_TERMINATED;
        // Total time of execution. Current time - amount of execution time
        current_task->executionTime += (systime() - current_task->executionTime);
        // Print of values
        printf("Dispatcher Task %d: execution time %dms, processor time %dms, %d activations.\n", current_task->id, current_task->executionTime, current_task->processingTime, current_task->activations);
#ifdef DEBUG
        debug_print("PPOS: dispatcher_body()=> Number of user tasks: %d, last task id was: %d with status %d. Exiting program.\n", user_tasks, last_id, current_task->status);
#endif
        // Exit with success
        exit(0);
    }
}

// Define the signal handler function with the signal number as its parameter
void handler(int signum)
{
    // Increase task time of execution
    current_task->processingTime++;
    // Increase system time
    system_time++;
    // Check if the current task has preemption
    if (current_task->preemption == TRUE)
    {
        task_timer--;
        // Check if the task timer has reached zero or below
        if (task_timer == 0)
        {
#ifdef DEBUG
            debug_print("PPOS: handler()=> Task %d has preemption. Resetting the task timer.\n", current_task->id);
#endif
            // Yield the current task to another one
            task_yield();
        }
        else
        {
            // If in debug mode, print a message with the remaining task timer ticks
#ifdef DEBUG
            debug_print("PPOS: handler()=> Task %d still has %d ticks.\n", current_task->id, task_timer);
#endif
            // Continue executing the current task
            return;
        }
    }
    else
    {
#ifdef DEBUG
        debug_print("PPOS: handler()=> Task %d is not preemption compatible.\n", current_task->id);
#endif
        // Continue executing the current task
        return;
    }
}

void timer_init()
{
    // Assign the 'handler' function as the signal handler for the action struct
    action.sa_handler = handler;
    // Initialize the action.sa_mask set to be an empty set
    sigemptyset(&action.sa_mask);
    // Set the flags of the action struct to 0
    action.sa_flags = 0;
    // Set up the signal handler for SIGALRM using the action struct and check if the sigaction call is successful
    if (sigaction(SIGALRM, &action, 0) < 0)
    {
        // If the sigaction call fails, print an error message
        perror("ERROR: ppos_init()=> Error on sigaction!\n");
        // Exit with error code
        exit(1);
    }
    // Set the initial timer value for the microsecond field
    timer.it_value.tv_usec = TEMPORIZER;
    // Set the initial timer value for the second field
    timer.it_value.tv_sec = 0;
    // Set the timer interval value for the microsecond field
    timer.it_interval.tv_usec = TEMPORIZER;
    // Set the timer interval value for the second field
    timer.it_interval.tv_sec = 0;
    // Set the timer using the ITIMER_REAL timer type and the timer struct and check if the setitimer call is successful
    if (setitimer(ITIMER_REAL, &timer, 0) < 0)
    {
        // If the setitimer call fails, print an error message
        perror("ERROR: ppos_init()=> Error on settimer!\n");
        // Exit with error code
        exit(1);
    }
}

// This function initializes the PingPongOS. It must be called at the beginning of the main function.
void ppos_init()
{
    // Initialize the clock of the system
    system_time = 0;
    // Disable buffering for stdout so that printed text appears immediately.
    setbuf(stdout, NULL);
    // Initialize Last Task ID
    last_id = 0;
    // Current number of user tasks
    user_tasks = 1;
    // ID of the main task
    main_task.id = 0;
    // Main task is ready
    main_task.status = TASK_READY;
    // Main task has preemption
    main_task.preemption = TRUE;
    // Set the number of activations
    main_task.activations = 1;
    // Set the processor usage time
    main_task.executionTime = 0;
    // Set the current task to the main task
    current_task = &main_task;
    // Save the current context into the "context" attribute of the main task.
    getcontext(&(main_task.context));
    // Append the main task to the queue of tasks
    queue_append((queue_t **)&tasks_queue, (queue_t *)&main_task);
#ifdef DEBUG
    debug_print("PPOS: ppos_init()=> Starting the system. Main task id: %d, number of user tasks: %d.\n", last_id, user_tasks);
#endif
    // Initialize the dispatcher task.
    task_init(&dispatcher, dispatcher_body, NULL);
    // Reset the preemption of the dispatcher task
    dispatcher.preemption = FALSE;
    // Initialize the timer
    timer_init();
    // Set the task timer to its initial value
    task_timer = TASK_TIMER;
}

// Initialize a new Task. Returns <ID> of the new stack or ERROR CODE
int task_init(task_t *task, void (*start_func)(void *), void *arg)
{
    // if get current context for the task returned with error
    if (getcontext(&(task->context)) == -1)
    {
        // Print error message
        perror("ERROR: task_init()=> Failed to get context!\n");
        // Exit with error code
        exit(1);
    }
    // Allocate memory for the task's stack and set the stack attributes
    void *stack = malloc(STACKSIZE);
    // If could not allocate
    if (stack == NULL)
    {
        // Print error message
        perror("ERROR: task_init()=> Stack allocation failed!\n");
        // Exit with error code
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
    // Set the dynamic and static priority of the task, (default = 0)
    task_setprio(task, 0);
    // Update the task's status
    task->status = TASK_READY;
    // Update the last_ID
    task->id = ++last_id;
    // Set the preemption to TRUE
    task->preemption = TRUE;
    // Set the number of activations
    task->activations = 0;
    // Set the execution time
    task->executionTime = 0;
    // Set the processor usage time
    task->processingTime = 0;
    // Set the sleeping time
    task->sleepingTime = 0;
    // Set the queue of waiting suspended tasks
    task->suspended = NULL;
    // Check if the task it is not the dispatcher
    if (task->id != dispatcher.id)
    {
        // Increment the user_tasks counter
        user_tasks++;
        // Append the task to the queue of tasks
        queue_append((queue_t **)&tasks_queue, (queue_t *)task);
#ifdef DEBUG
        debug_print("PPOS: task_init()=> Task created with id %d, currently there are %d user tasks.\n", last_id, user_tasks);
        queue_print("PPOS: task_init()=> Queue of tasks: ", (queue_t *)(tasks_queue), print_element);
#endif
    }
    // Configure the task's context to execute 'start_func' with 'arg' when scheduled
    makecontext(&(task->context), (void (*)(void))start_func, 1, arg);
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
    // Suspend the current task
    current_task->status = TASK_SUSPENDED;
    // Dispatcher ready to execute
    dispatcher.status = TASK_READY;
    // Switch the execution to the dispatcher task, which will handle task scheduling.
    task_switch(&dispatcher);
}

// Switches execution to the indicated task "task"
int task_switch(task_t *task)
{
    // If the given task is NULL, return an error code (1)
    if (!task)
    {
        // Print error message
        perror("ERROR: task_switch()=> task is a NULL Pointer!\n");
        // Exit with error code
        exit(1);
    }
    // Store the previous task before switching
    task_t *previous_task = current_task;
    // Update the current task pointer to the new task
    current_task = task;
    // Update the status of the previous task
    if (previous_task->status != TASK_TERMINATED)
    {
        // If it was not terminated put it on state TASK_SUSPENDED
        previous_task->status = TASK_SUSPENDED;
    }
    // Set the new task's status to TASK_RUNNING
    current_task->status = TASK_RUNNING;
#ifdef DEBUG
    debug_print("PPOS: task_switch()=> Switching tasks from: %d to: %d\n", previous_task->id, current_task->id);
#endif
    // Perform the context switch between the previous task and the new task
    int result = swapcontext(&(previous_task->context), &(current_task->context));
    // If the context switch was not successful
    if (result != 0)
    {
        // Print error message
        perror("ERROR: task_switch()=> Could not swap context!\n");
        // Exit with error code
        exit(1);
    }
    else // Context switch was successfull
    {
        // Return success
        return result;
    }
}

// Get the value of the static priority of the task "task" or the current task
int task_getprio(task_t *task)
{
    // If task "task" is a NULL pointer
    if (!task)
    {
        // Return the current task static priority
        return current_task->staticPriority;
    }
    else // In case "task" is a valid pointer
    {
        // Return the task static priority of the specified task
        return task->staticPriority;
    }
}

// Adjust the priority of the "task" to the value of "prio"
void task_setprio(task_t *task, int prio)
{
    // Check if the 'prio' value is outside the valid range of -20 to 20
    if (prio < -20 || prio > 20)
    {
        // If 'prio' is less than -20
        if (prio < -20)
        {
            // Set 'prio' to the minimum allowed value (-20)
            prio = -20;
        }
        // If 'prio' is greater than 20
        else
        {
            // Set 'prio' to the maximum allowed value (20)
            prio = 20;
        }
    }
    // If task "task" is a NULL pointer
    if (!task)
    {
        // Change both prioritys of the current task
        current_task->dynamicPriority = prio;
        current_task->staticPriority = prio;
    }
    else // task is not a NULL Pointer
    {
        // Change both prioritys of the task
        task->staticPriority = prio;
        task->dynamicPriority = prio;
    }
}

// Suspend the current task
void task_suspend(task_t **queue)
{
    // If the given queue is NULL, return an error code (1)
    if (!queue)
    {
        // Print error message
        perror("ERROR: task_suspend()=> queue is a NULL Pointer!\n");
        // Exit with error code
        exit(1);
    }
    // Remove the current task from the tasks_queue
    if (queue_remove((queue_t **)(&tasks_queue), (queue_t *)current_task) != 0)
    {
#ifdef DEBUG
        // Print debug message if task removal failed
        printf("PPOS: task_suspend()=> Failed to remove task %d from tasks_queue.\n", current_task->id);
#endif
        // Exit the program if task removal failed
        exit(1);
    }
    // Update the status of the current task to 'TASK_SUSPENDED'
    current_task->status = TASK_SUSPENDED;
    // Append the current task to the specified queue
    if (queue_append((queue_t **)queue, (queue_t *)current_task) != 0)
    {
#ifdef DEBUG
        // Print debug message if task append failed
        printf("PPOS: task_suspend()=> Failed to append task %d to the suspended queue of tasks.\n", current_task->id);
#endif
        // Exit the program if task append failed
        exit(1);
    }
    // Switch control to the dispatcher
    task_yield();
}

// Resume the specified task
void task_resume(task_t *task, task_t **queue)
{
    // If the given task is NULL, return an error code (1)
    if (!task)
    {
        // Print error message
        perror("ERROR: task_resume()=> task is a NULL Pointer!\n");
        // Exit with error code
        exit(1);
    }
    // If the given queue is NULL, return an error code (1)
    if (!queue)
    {
        // Print error message
        perror("ERROR: task_resume()=> queue is a NULL Pointer!\n");
        // Exit with error code
        exit(1);
    }
    // Remove the current task from the tasks_queue
    if (queue_remove((queue_t **)queue, (queue_t *)task) != 0)
    {
#ifdef DEBUG
        // Print debug message if task removal failed
        printf("PPOS: task_resume()=> Failed to remove task %d from suspended queue.\n", current_task->id);
#endif
        // Exit the program if task removal failed
        exit(1);
    }
    task->status = TASK_READY;
    if (queue_append((queue_t **)(&tasks_queue), (queue_t *)task) != 0)
    {
#ifdef DEBUG
        // Print debug message if task append failed
        printf("PPOS: task_resume()=> Failed to append task %d to the tasks_queue.\n", current_task->id);
#endif
        // Exit the program if task append failed
        exit(1);
    }
}

int task_wait(task_t *task)
{
    // Check if the given task is null
    if (!task)
    {
        // Print an error message indicating that the task is null
        perror("ERROR: task_wait()=> task is a NULL Pointer!\n");
        // Terminate the program with error status
        exit(1);
    }
    // If the task to wait on is the current task, return error code (-1)
    if (current_task->id == task->id)
    {
        return -1;
    }
    // If the task is not already terminated, suspend it
    if (!(task->status == TASK_TERMINATED))
    {
        task_suspend(&task->suspended);
    }
    // Return the ID of the waited task
    return task->id;
}

void task_sleep(int t)
{
    // Check if the current_task is NULL
    if (!current_task)
    {
        // Print an error message indicating that the task is null
        perror("ERROR: task_sleep()=> current_task is a NULL Pointer!\n");
        // Terminate the program with error status
        exit(1);
    }
    // Set the amount of time the task will sleep
    current_task->sleepingTime = systime() + t;
    // Add the current_task to the queue of suspended tasks
    task_suspend(&suspended_tasks_queue);
    // Give control back to the dispatcher
    // task_yield();
}

// Ends the current task with a specified exit code
void task_exit(int exit_code)
{
    // Update the id of the last tasks
    last_id = current_task->id;
    // Change the status of the currently running task to TASK_TERMINATED
    current_task->status = TASK_TERMINATED;
    // Update the user tasks count by decrementing it by one
    --user_tasks;
    // Check if there are suspended tasks waiting for the current task
    if (current_task->suspended != NULL)
    {
        // Initialize a pointer to the first waiting suspended task
        task_t *aux = current_task->suspended;
        // Save the pointer to the first task as the starting point for the loop
        task_t *start = aux;
        // Declare a pointer to store the next task in the queue
        task_t *next_aux;
        // Start a loop that will go through each waiting suspended task in the queue
        do
        {
            // Store the next task before resuming the current one
            next_aux = aux->next;
            // Call the resume function for the current waiting task
            task_resume(aux, &current_task->suspended);
            // Move to the next waiting task in the queue
            aux = next_aux;
            // Continue looping as long as the current waiting task is not the one i started with
        } while (aux != start);
    }
    // Total time of execution. Current time - amount of execution time
    current_task->executionTime += (systime() - current_task->executionTime);
    // Print of values
    printf("Task %d exit: execution time %dms, processor time %dms, %d activations.\n", current_task->id, current_task->executionTime, current_task->processingTime, current_task->activations);
    // Remove task from the queue
    if (queue_remove((queue_t **)&tasks_queue, (queue_t *)current_task) != 0)
    {
#ifdef DEBUG
        printf("PPOS: task_exit()=> Failed to remove terminated task %d from tasks_queue\n", current_task->id);
#endif
    }
#ifdef DEBUG
    // Log the task termination for debugging purposes
    debug_print("PPOS: task_exit()=> Task %d terminated with exit code %d, with %d activations.\n", current_task->id, exit_code, current_task->activations);
    debug_print("PPOS: task_exit()=> Currently there are %d user tasks.\n", user_tasks);
    queue_print("PPOS: task_exit()=> Queue of tasks: ", (queue_t *)(tasks_queue), print_element);
#endif
    // Transfer control to the dispatcher
    task_switch(&dispatcher);
}

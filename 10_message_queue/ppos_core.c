// GRR20197155 Gabriel Razzolini Pires De Paula

// Including libs needed
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/time.h>
#include <string.h>
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

task_t main_task;        // Main Task: This variable represents the main task of the program.
task_t *current_task;    // Current running Task: This pointer points to the task that is currently running.
task_t *ready_tasks;     // Queue of tasks: This pointer represents the queue of all tasks, which includes tasks with different states (e.g., TASK_READY, TASK_RUNNING).
task_t *suspended_tasks; // Queue of Suspended Tasks: This pointer represents the queue of tasks that are in state "TASK_SUSPENDED" and were put to sleep.
task_t dispatcher;       // Dispatcher: This variable represents the dispatcher task, which is responsible for managing tasks execution and switching between them.

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
    // Check if there are tasks in the queue of "ready_tasks"
    if (ready_tasks)
    {
        // Set lowest_priority_task to the first task in the queue
        task_t *lowest_priority_task = ready_tasks;
        // Set tmp_task to the next task in the queue
        task_t *tmp_task = ready_tasks->next;
        // Set queue_start to the next task in the queue (same as tmp_task initially)
        task_t *queue_start = ready_tasks->next;
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
#ifdef DEBUG
    queue_print("PPOS: scheduler()=> Queue of ready tasks is empty! Queue", (queue_t *)ready_tasks, print_element);
#endif
    return NULL;
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

// This functions is used to wake up tasks that have slept for the specified time
void task_wakeup()
{
#ifdef DEBUG
    queue_print("PPOS: task_wakeup()=> Queue of sleeping tasks: ", (queue_t *)suspended_tasks, print_element);
#endif
    // Check if there are tasks in the queue of "suspended_tasks"
    if (suspended_tasks)
    {
        // Pointer to the head of suspended tasks queue
        task_t *start = suspended_tasks;
        // Pointer used to iterate over each task in the suspended tasks queue
        task_t *aux = start;
        // Pointer used to save the next task
        task_t *next;
        // Start a loop that will go through each suspended tasks of the queue
        do
        {
#ifdef DEBUG
            debug_print("PPOS: task_wakeup()=> head of the queue is task %d, current in task %d, next is %d.\n", start->id, aux->id, aux->next->id);
            queue_print("PPOS: task_wakeup()=> Queue of sleeping tasks: \n", (queue_t *)suspended_tasks, print_element);
#endif
            // If it is time for the task to wakeup
            if (systime() >= aux->sleepingTime)
            {
                // Save the next task before resuming
                next = aux->next;
#ifdef DEBUG
                debug_print("PPOS: task_wakeup()=> systime(): %d, wake up time of task %d: %d.\n", systime(), aux->id, aux->sleepingTime);
#endif
                // Call the resume function for the current waiting task
                task_resume(aux, &suspended_tasks);
                // If the queue is empty
                if (!suspended_tasks)
                {
                    // Exit the loop, therefore exiting this function
                    break;
                }
                // Reset the pointer to the head of suspended tasks queue
                start = suspended_tasks;
                // Move to the next task after potentially altering the queue
                aux = next;
            }
            else
            {
                // If didn't resume the task, just move to the next task in the queue
                aux = aux->next;
            }
            // Continue looping as long as the current suspended task is not the one we started with
        } while (aux != start);
    }
}

// The dispatcher_body function is responsible for managing the execution of user tasks.
void dispatcher_body()
{
    // Set the status of the dispatcher task
    dispatcher.status = TASK_RUNNING;
    // Create new pointer variable to represent the next task
    task_t *next_task;
    // The dispatcher loop continues running until there are no more user tasks.
    while (user_tasks > 0)
    {
        // Get the next task to be executed using the scheduler function.
        next_task = scheduler();
        // Wake up tasks that should
        task_wakeup();
        // If there is a next task to execute
        if (next_task)
        {
            // Reset the attributes of the next_task
            task_reset(next_task);
            // Increase the amount of activations of the task
            next_task->activations++;
#ifdef DEBUG
            debug_print("PPOS: dispatcher_body()=> Dispatcher activations: %d times.\n", dispatcher.activations);
            debug_print("PPOS: dispatcher_body()=> Number of user tasks: %d, next task id: %d, next_task activations: %d.\n", user_tasks, next_task->id, next_task->activations);
#endif
            // Perform a context switch to the next task.
            task_switch(next_task);
            // Add processing time to the execution time
            next_task->executionTime += next_task->processingTime;
            // Handle the status of task "next_task" after execution
            switch (next_task->status)
            {
            case TASK_READY:
                break;
            case TASK_SUSPENDED:
                break;
            }
        }
#ifdef DEBUG
        queue_print("PPOS: dispatcher_body()=> Queue of tasks: ", (queue_t *)ready_tasks, print_element);
#endif
    }
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
        // Decrement the timer of the task
        task_timer--;
        // Check if the task timer has reached zero or below
        if (task_timer == 0)
        {
            // Yield the current task to another one
            task_yield();
        }
        else
        {
            // Continue executing the current task
            return;
        }
    }
    else
    {
        // Continue executing the current task
        return;
    }
}

// Setup function to use SIGALARM
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
    queue_append((queue_t **)&ready_tasks, (queue_t *)&main_task);
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
    // Give control to the dispatcher
    task_yield();
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
    // Check if the task it is not the dispatcher
    if (task->id != dispatcher.id)
    {
        // Increment the user_tasks counter
        user_tasks++;
        // Append the task to the queue of tasks
        queue_append((queue_t **)&ready_tasks, (queue_t *)task);
#ifdef DEBUG
        debug_print("PPOS: task_init()=> Task created with id %d, currently there are %d user tasks.\n", last_id, user_tasks);
        queue_print("PPOS: task_init()=> Queue of tasks: ", (queue_t *)(ready_tasks), print_element);
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

// This function gives control back to the Dispatcher
void task_yield()
{
    // Increase the amount of activations of the dispatcher
    dispatcher.activations++;
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

// Get the value of the static priority of the task "task" or the current task, if "task" is not defined
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
        // Change both prioritys(dynamic and static) of the current task
        current_task->dynamicPriority = prio;
        current_task->staticPriority = prio;
    }
    else // task is not a NULL Pointer
    {
        // Change both prioritys(dynamic and static) of the task
        task->dynamicPriority = prio;
        task->staticPriority = prio;
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
    // Remove the current task from the ready_tasks
    if (queue_remove((queue_t **)(&ready_tasks), (queue_t *)current_task) != 0)
    {
#ifdef DEBUG
        // Print debug message if task removal failed
        printf("PPOS: task_suspend()=> Failed to remove task %d from ready_tasks.\n", current_task->id);
#endif
        // Exit the program if task removal failed
        exit(1);
    }
#ifdef DEBUG
    // Print debug message if task removal failed
    printf("PPOS: task_suspend()=> task %d removed from ready_tasks.\n", current_task->id);
    queue_print("PPOS: task_suspend()=> Queue of ready tasks: ", (queue_t *)ready_tasks, print_element);
#endif
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
#ifdef DEBUG
    // Print debug message if task append failed
    printf("PPOS: task_suspend()=> task %d appended to the suspended queue of tasks.\n", current_task->id);
    queue_print("PPOS: task_suspend()=> Queue of sleeping tasks: ", (queue_t *)suspended_tasks, print_element);
#endif
    // Switch control to the dispatcher
    task_yield();
}

// Resume the specified task "task"
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
    // Remove the current task from the ready_tasks
    if (queue_remove((queue_t **)queue, (queue_t *)task) != 0)
    {
#ifdef DEBUG
        // Print debug message if task removal failed
        printf("PPOS: task_resume()=> Failed to remove task %d from suspended queue.\n", task->id);
#endif
        // Exit the program if task removal failed
        exit(1);
    }
#ifdef DEBUG
    // Print debug message if task append failed
    printf("PPOS: task_resume()=> task %d removed from the suspended queue of tasks.\n", task->id);
    queue_print("PPOS: task_resume()=> Queue of sleeping tasks: ", (queue_t *)suspended_tasks, print_element);
#endif
    task->status = TASK_READY;
    if (queue_append((queue_t **)(&ready_tasks), (queue_t *)task) != 0)
    {
#ifdef DEBUG
        // Print debug message if task append failed
        printf("PPOS: task_resume()=> Failed to append task %d to the ready_tasks.\n", task->id);
#endif
        // Exit the program if task append failed
        exit(1);
    }
#ifdef DEBUG
    // Print debug message if task append failed
    printf("PPOS: task_resume()=> task %d appended to the ready_tasks.\n", task->id);
    queue_print("PPOS: task_resume()=> Queue of ready tasks: ", (queue_t *)ready_tasks, print_element);
#endif
}

// Suspend the current task until the give task "task" is terminated
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
#ifdef DEBUG
    debug_print("PPOS: task_wait()=> task %d is going to wait\n.", task->id);
#endif
    // Return the ID of the waited task
    return task->id;
}

// This function set the amount of time the current task should sleep
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
#ifdef DEBUG
    debug_print("PPOS: task_sleep()=> current_task %d is going to sleep for %dms.\n", current_task->id, current_task->sleepingTime);
#endif
    // Add the current_task to the queue of suspended tasks
    task_suspend(&suspended_tasks);
}

// Ends the current task with a specified exit code and awake the waiting tasks on their queue
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
    if (queue_remove((queue_t **)&ready_tasks, (queue_t *)current_task) != 0)
    {
#ifdef DEBUG
        printf("PPOS: task_exit()=> Failed to remove terminated task %d from ready_tasks\n", current_task->id);
#endif
    }
#ifdef DEBUG
    // Log the task termination for debugging purposes
    debug_print("PPOS: task_exit()=> terminated task %d removed from ready_tasks\n", current_task->id);
    debug_print("PPOS: task_exit()=> Task %d terminated with exit code %d, with %d activations.\n", current_task->id, exit_code, current_task->activations);
    debug_print("PPOS: task_exit()=> Currently there are %d user tasks.\n", user_tasks);
    queue_print("PPOS: task_exit()=> Queue of tasks: ", (queue_t *)(ready_tasks), print_element);
#endif
    // Increase the amount of activations of the dispatcher
    dispatcher.activations++;
    // Transfer control to the dispatcher
    task_switch(&dispatcher);
}

/************************************************************ SEMAPHORE ************************************************************/

// Enter critical section
void enter_cs(int *lock)
{
    // Atomic OR (Intel macro for GCC)
    while (__sync_fetch_and_or(lock, 1))
        ;
}

// Leave critical section
void leave_cs(int *lock)
{
    // Free the lock
    (*lock) = 0;
}

// Create a semaphore "s"
int sem_init(semaphore_t *s, int value)
{
    s->lock = 0;        // Initializes the semaphore lock to 0 indicating it is available.
    s->counter = value; // Sets the semaphore's counter to the provided value.
    s->active = ACTIVE; // Marks the semaphore as active.
    s->queue = NULL;    // Initializes the semaphore's queue to NULL, indicating no tasks are waiting.
    return 0;           // Returns 0 to signify successful semaphore initialization.
}

// Request the use of the semaphore "s"
int sem_down(semaphore_t *s)
{
    // Checks if the semaphore pointer is NULL
    if (!s)
    {
        // Prints an error message
        perror("ERROR: sem_down()=> semaphore pointed is NULL!.\n");
        // Returns with an error code
        return -1;
    }
    // Checks if the semaphore is INACTIVE
    if (s->active == INACTIVE)
    {
        // Prints an error message
        perror("ERROR: sem_down()=> semaphore pointed is INACTIVE!.\n");
        // Returns with an error code
        return -1;
    }
    // Enter critical section, locking the semaphore
    enter_cs(&(s->lock));
    // Decrement the semaphore's counter
    s->counter--;
    // Leave the critical section, unlocking the semaphore
    leave_cs(&s->lock);
    // If the semaphore's counter is less than zero
    if (s->counter < 0)
    {
        // Suspend the current task and add it to the semaphore queue
        task_suspend(&(s->queue));
    }
    // Returns zero to indicate success
    return 0;
}

// Release the semaphore "s"
int sem_up(semaphore_t *s)
{
    // Checks if the semaphore pointer is NULL
    if (!s)
    {
        // Prints an error message
        perror("ERROR: sem_up()=> semaphore pointed is NULL!.\n");
        // Returns with an error code
        return -1;
    }
    // Checks if the semaphore is INACTIVE
    if (s->active == INACTIVE)
    {
        // Returns with an error code
        return -1;
    }
    // Enter critical section, locking the semaphore
    enter_cs(&(s->lock));
    // Increment the semaphore's counter
    s->counter++;
    // Leave the critical section, unlocking the semaphore
    leave_cs(&(s->lock));
    // If the head of the queue exists or the counter is less/equal than 0
    if (s->queue || s->counter <= 0)
    {
        // Resume the head of the queue
        task_resume(s->queue, &(s->queue));
    }
    // Returns zero to indicate success
    return 0;
}

// Destroy the specified semaphore "s"
int sem_destroy(semaphore_t *s)
{
    // Checks if the semaphore pointer is NULL
    if (!s)
    {
        // Prints an error message
        perror("ERROR: sem_destroy()=> semaphore pointed is already NULL!.\n");
        // Returns with an error code
        return -1;
    }
    // Checks if the semaphore is INACTIVE
    if (s->active == INACTIVE)
    {
        // Prints an error message
        perror("ERROR: sem_destroy()=> semaphore pointed is already INACTIVE!.\n");
        // Returns with an error code
        return -1;
    }
    // Enter critical section, locking the semaphore
    enter_cs(&(s->lock));
    // Sets the semaphore's state to INACTIVE
    s->active = INACTIVE;
    // Set the semaphore counter to INACTIVE
    s->counter = INACTIVE;
    // Loops through all the tasks in the semaphore's queue
    while (s->queue)
    {
        // Resumes the head task of the queue
        task_resume(s->queue, &s->queue);
    }
    // Resets the semaphore's queue
    s->queue = NULL;
    // Leave the critical section, unlocking the semaphore
    leave_cs(&(s->lock));
    // Returns zero to indicate success
    return 0;
}

/************************************************************ MESSAGE QUEUE ************************************************************/
// Initialize a Message Queue, returning 0 on success or -1 otherwise
#define ERR_QUEUE_NULL -2
#define ERR_QUEUE_INACTIVE -3
#define ERR_MEM_ALLOC -4

// Initialize a message queue.
int mqueue_init(mqueue_t *queue, int max_msgs, int msg_size)
{
    if (!queue)
        return ERR_QUEUE_NULL;

    queue->data_buffer = malloc(msg_size * max_msgs);
    if (queue->data_buffer == NULL)
    {
        return ERR_MEM_ALLOC; // memory allocation failed
    }

    queue->max_msg = max_msgs;
    queue->size_msg = msg_size;
    queue->status = ACTIVE;
    queue->last_position = queue->last_item = -1;
    sem_init(&queue->buffer, 1);
    sem_init(&queue->item, 0);
    sem_init(&queue->vaga, max_msgs);
    return 0;
}

int mqueue_send(mqueue_t *queue, void *msg)
{
    if (!queue)
        return ERR_QUEUE_NULL;

    if (queue->status == INACTIVE)
        return ERR_QUEUE_INACTIVE;

    sem_down(&queue->vaga);
    sem_down(&queue->buffer);

    queue->last_position = (queue->last_position + 1) % queue->max_msg;

    void *buffer_position = queue->data_buffer + queue->last_position * queue->size_msg;
    memcpy(buffer_position, msg, queue->size_msg);

    if (queue->last_position == queue->last_item)
        queue->is_full = TRUE;

    sem_up(&queue->item);
    sem_up(&queue->buffer);

    return 0;
}

int mqueue_recv(mqueue_t *queue, void *msg)
{
    if (!queue)
        return ERR_QUEUE_NULL;

    if (queue->status == INACTIVE)
        return ERR_QUEUE_INACTIVE;

    sem_down(&queue->item);
    sem_down(&queue->buffer);

    queue->is_full = FALSE;
    queue->last_item = (queue->last_item + 1) % queue->max_msg;

    void *buffer_position = queue->data_buffer + queue->last_item * queue->size_msg;
    memcpy(msg, buffer_position, queue->size_msg);

    sem_up(&queue->vaga);
    sem_up(&queue->buffer);

    return 0;
}

// Destroy a message queue.
int mqueue_destroy(mqueue_t *queue)
{
    if (!queue)
        return ERR_QUEUE_NULL;

    if (queue->status == INACTIVE)
        return ERR_QUEUE_INACTIVE;

    queue->status = INACTIVE;
    free(queue->data_buffer);
    sem_destroy(&queue->vaga);
    sem_destroy(&queue->item);
    sem_destroy(&queue->buffer);

    return 0;
}

int mqueue_msgs(mqueue_t *queue)
{
    if (!queue)
        return ERR_QUEUE_NULL;

    if (queue->status == INACTIVE)
        return ERR_QUEUE_INACTIVE;

    if (queue->last_position == queue->last_item)
        return (queue->is_full) ? queue->max_msg : 0;

    if (queue->last_position > queue->last_item)
        return queue->last_position - queue->last_item;

    return queue->max_msg - (queue->last_item - queue->last_position);
}
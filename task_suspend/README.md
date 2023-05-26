# Suspended Tasks

This project's aim is to construct functions to suspend and awaken tasks in a multi-tasking environment.

## Main Function

The primary function to implement is task_wait, which allows a task to suspend itself while awaiting the completion of another task. This is analogous to POSIX's wait and pthread_join calls.

Here is the declaration for task_wait:

`int task_wait (task_t *task);`

## Functionality

The call to task_wait(b) causes the current task to be suspended until task b completes. When task b concludes its operation (by invoking the task_exit call), the suspended task should return to the queue of ready tasks.

Note that several tasks may be waiting for task b to finish. All these tasks must be awakened when b ends its operation.

If task b either doesn't exist or has already ended, task_wait should return immediately, without suspending the current task.

## Return Value

The task_wait call should return the termination code of task b (the exit_code value provided as a parameter of task_exit). If the indicated task doesn't exist or if there's another error, the function should return -1.
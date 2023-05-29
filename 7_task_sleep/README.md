# Sleeping Tasks

In UNIX, the system call **`sleep(t)`** suspends a process for t seconds. In this project, I've implemented a similar function named **`task_sleep`**(). This function suspends the current task for the specified interval (expressed in milliseconds), without disrupting the execution of other tasks.

To implement this functionality, I've done the following:

- **Created a separate queue for "sleeping tasks":** This is separate from the ready tasks queue.

- **Implemented the `task_sleep` function:** This function calculates the instance when the current task should wake up and suspends it in the queue of sleeping tasks using the task_suspend function.

- **Modified the dispatcher to periodically traverse the queue of sleeping tasks:** The dispatcher activates tasks that are due to wake up using the **`task_resume`** function.

- **Implemented preemption control:** To avoid race conditions in the manipulation of the sleeping tasks queue, preemption control was put in place.

By following these steps, **`task_sleep`** function allows tasks to pause their execution for a specified amount of time without disturbing the execution of other tasks. This function is now a crucial part of the task management system within this project.
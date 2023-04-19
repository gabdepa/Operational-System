# Priority Scheduler
This project goal is to add a scheduler based on priority with aging to my OS.

2 new functions were added:

``` void task_setprio (task_t *task, int prio) ```
This functions adjusts the static priority of the task "**task**" to the value "**prio**" which must be a number between -20 and +20. In the case "**task**" be NULL, adjusts the priority to the current task.

``` int task_getprio (task_t *task) ```
This function return static priority value of the task "**task**", or of the current task, in case "**task**" is NULL.

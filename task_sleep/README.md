# Task Sleep

In UNIX, the system call `sleep(t)` suspends a process for `t` seconds. In this project, I've successfully created a similar function, `task_sleep`. This function suspends the current task for a specified duration (time in milliseconds), without interrupting the execution of other tasks:

`void task_sleep(int t);`

The time passed to **task_sleep** is expressed in milliseconds.

Here is how I implemented this functionality:

- I created a separate queue of "sleeping tasks" that is distinct from the ready tasks queue;
- I developed the **`task_sleep`** function. This function calculates when the current task should be awakened and suspends it in the queue of sleeping tasks, utilizing **`task_suspend`**;
- Periodically, the dispatcher traverses the queue of sleeping tasks and reactivates those tasks that are ready to wake up, using **`task_resume`**;
- I employed preemption control to avoid race conditions when manipulating the queue of sleeping tasks.

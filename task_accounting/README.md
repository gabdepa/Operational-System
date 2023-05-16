# Tasks Accounting

The figure below illustrates the execution of a specific task, from its creation (**task_init**) to its termination (**task_exit**). The green areas indicate processor usage. It is easy to see how the accounting values can be calculated:

![Schema](https://wiki.inf.ufpr.br/maziero/lib/exe/fetch.php?cache=&media=so:contabilizacao.png)

To perform accounting, a time reference (a clock) was needed. For this purpose, a global variable was defined to count clock ticks, incremented with each timer interruption (1 ms). Thus, this variable indicates the number of ticks elapsed since the system initialization in the **ppos_init** function, acting as a millisecond-based clock.

A function was implemented to inform tasks of the current value of the clock:

`unsigned int systime () ;`



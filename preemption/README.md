# Preemption

So far, my system only supports cooperative tasks. The goal of this project is to add time preemption to the system. With this modification, my system will now support preemptive tasks, which can switch processor usage without the need for explicit context switches via **task_yield**.

The image below describes this:


![Schema](https://wiki.inf.ufpr.br/maziero/lib/exe/fetch.php?cache=&media=so:time-sharing.png)

# Semaphore Implementation

The purpose of this project was to implement classic semaphores in our system. The successfully implemented functions are described below.

## Creating a Semaphore
**`int sem_init (semaphore_t *s, int value);`**

I initialized a semaphore pointed by **`s`** with the initial **`value`** and an empty queue. The **`semaphore_t`** type was defined in the ppos_data.h file.
This function call returns 0 in case of success or -1 in case of an error.

## Requesting a Semaphore

**`int sem_down (semaphore_t *s);`**

I implemented the Down operation on the semaphore pointed by **`s`**. This call can be blocking: if the semaphore counter is negative, the current task is suspended, inserted at the end of the semaphore queue, and the execution returns to the dispatcher. Otherwise, the task continues to execute without being suspended.

If the task is blocked, it will be reactivated when another task releases the semaphore (through the **`sem_up`** operation) or if the semaphore is destroyed (**`sem_destroy`** operation).

This function call returns 0 in case of success or -1 in case of an error (semaphore does not exist or was destroyed).

## Releasing a Semaphore

**`int sem_up (semaphore_t *s);`**

I implemented the Up operation on the semaphore pointed by **`s`**. This call is not blocking (the task that executes it does not lose the processor). If there are tasks waiting in the semaphore queue, the first one in the queue is woken up and returned to the ready tasks queue.

This function call returns 0 in case of success or -1 in case of an error (semaphore does not exist or was destroyed).

## Destroying a Semaphore

**`int sem_destroy (semaphore_t *s);`**

I destroyed the semaphore pointed by **`s`**, waking up all the tasks that were waiting for it.

This function call returns 0 in case of success or -1 in case of an error.

By implementing these functions, I've added semaphore support to our system, providing a means of synchronizing access to resources and effectively managing concurrent tasks.

## Handling Race Conditions

In the development process, I recognized the potential for race conditions if two threads tried to access the same semaphore simultaneously. Such scenarios could lead to inconsistencies in the internal variables of the semaphores. To tackle this, I ensured that the functions implementing the semaphores were protected using a mechanism of mutual exclusion.

Since it was clear that we couldn't use semaphores to solve this problem (due to the potential recursive conflict), I resorted to a more primitive mechanism based on busy waiting. For this purpose, I employed atomic operations to ensure thread-safe access to the semaphores.

This atomic operations are implemented in functions:

**`void enter_cs(int *lock);`**
&
**`void leave_cs(int *lock);`**

Through this approach, I was able to mitigate the risks of race conditions and ensure the robust and reliable functioning of semaphore operations within concurrent threads.
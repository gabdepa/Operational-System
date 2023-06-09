// Internal structures of the Operational System
#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h> // POSIX Librarie of context switchs

// Status value
#define TASK_READY 0
#define TASK_RUNNING 1
#define TASK_SUSPENDED 2
#define TASK_TERMINATED 3
#define NEW_TASK 4

// Timer of the task
#define TASK_TIMER 20

// Temporizer
#define TEMPORIZER 1000

// Boolean
#define TRUE 1
#define FALSE 0

// Semaphore use
#define ACTIVE 1
#define INACTIVE 0

// Size of threads stack
#define STACKSIZE 64 * 1024

// Message Queue use
#define ERR_QUEUE_NULL -2
#define ERR_QUEUE_INACTIVE -3
#define ERR_MEM_ALLOC -4

// Structure that defines a Task Control Block (TCB)
typedef struct task_t
{
  struct task_t *prev, *next; // Next and Previous elements of the queue
  int id;                     // Identificator of the Task
  short status;               // TASK_READY, TASK_RUNNING, TASK_SUSPENDED, ...
  short int staticPriority;   // Static priority of the task
  short int dynamicPriority;  // Dynamic priority of the task
  ucontext_t context;         // Context storage in the task
  short int preemption;       // Preemption: True or False
  int timer;                  // Timer of the task
  unsigned int activations;   // Number of times the task was activated
  unsigned int executionTime; // Amount of time executing
  unsigned processingTime;    // Amount of time using the processor
  unsigned int sleepingTime;  // Amount of time the task will sleep
  struct task_t *suspended;   // Queue for suspended tasks that are waiting a certain task to terminate
} task_t;

// Structure that defines a Semaphore
typedef struct
{
  int lock;      // Lock that indicates if the critical zone is being used
  int counter;   // Count how many tasks are waiting in the queue
  int active;    // Indicates if the semaphore is ACTIVE or INACTIVE
  task_t *queue; // Queue of tasks waiting to access the critical zone
} semaphore_t;

// Structure that defines a Message Queue
typedef struct
{
  semaphore_t item;     // Item: This semaphore is used to keep track of the number of items in the queue. It is incremented after a message is sent to the queue, and decremented after a message is received from the queue.
  semaphore_t vacancy;  // Vacancy: This semaphore is used to keep track of the number of free spaces in the queue. It is decremented after a message is sent to the queue and incremented after a message is received from the queue.
  semaphore_t buffer;   // Buffer:  This semaphore is used to ensure mutual exclusion while accessing the queue buffer during send and receive operations, preventing data corruption due to concurrent access.
  int msg_size;         // Size of a Message: The size of each message in the queue. This is used when allocating memory for the buffer, sending, and receiving messages.
  int max_msgs;         // Maximum Messages: This is used to calculate buffer memory size, and manage the queue's full and empty states.
  int status;           // Status: Queue Status(ACTIVE, INACTIVE). This flag is checked before performing operations like sending, receiving, or destroying the queue.
  int last_insertion;   // Last Insertion point in the queue: Determines the latest point of insertion on the queue. This is used to track the latest position a message was added to the queue, which helps to manage the queue as a circular buffer.
  int last_consumption; // Last Consumption point in the queue: Determines the latest point of comsumption on the queue. This is used to track the latest position a message was removed from the queue, which helps to manage the queue as a circular buffer.
  unsigned int is_full; // "Is Full?": Verify if the queue is Full.
  void *buffer_data;    // Buffer Data: Queue buffer that holds all the data in the Message Queue
} mqueue_t;

// Structure that defines a Mutex
typedef struct
{
  // preencher quando necessário
} mutex_t;

// Structure that defines a Barrier
typedef struct
{
  // preencher quando necessário
} barrier_t;

#endif

// Estruturas de dados internas do sistema operacional
#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h> // biblioteca POSIX de trocas de contexto

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
#define TRUE 0

// Size of threads stack
#define STACKSIZE 64 * 1024

// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
  struct task_t *prev, *next; // ponteiros para usar em filas
  int id;                     // identificador da tarefa
  short status;               // TASK_READY, TASK_RUNNING, TASK_SUSPENDED, ...
  short int staticPriority;          // static priority of the task
  short int dynamicPriority;         // dynamic priority of the task
  ucontext_t context;         // contexto armazenado da tarefa
  short int preemption;       // Preemption: True or False
  int timer;             // Timer of the task
} task_t;

// estrutura que define um semáforo
typedef struct
{
  // preencher quando necessário
} semaphore_t;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t;

#endif

#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

#define TAM_BUFFER 10

int last_insertion = -1;
int last_consumed = -1;

int buffer[TAM_BUFFER];

task_t prod1, prod2, prod3, cons1, cons2;
semaphore_t s_buffer, s_item, s_vacancy;

void produtor(void *arg)
{
    int item;
    while (1)
    {
        task_sleep(1000);
        item = rand() % 100;
        sem_down(&s_vacancy);
        sem_down(&s_buffer);
        last_insertion++;
        if (last_insertion == (TAM_BUFFER + 1))
        {
            last_insertion = 0;
        }
        buffer[last_insertion] = item;
        printf("%s produziu %d\n", (char *)arg, item);
        sem_up(&s_buffer);
        sem_up(&s_item);
    }
}

void consumidor(void *arg)
{
    int item;
    while (1)
    {
        sem_down(&s_item);
        sem_down(&s_buffer);
        last_consumed++;
        if (last_consumed == (TAM_BUFFER + 1))
        {
            last_consumed = 0;
        }
        item = buffer[last_consumed];
        printf("%s consumiu %d\n", (char *)arg, item);
        sem_up(&s_buffer);
        sem_up(&s_vacancy);
    }
}

int main()
{
    printf("main : início\n");
    ppos_init();
    sem_init(&s_buffer, 1);
    sem_init(&s_item, 0);
    sem_init(&s_vacancy, 5);
    task_init(&prod1, produtor, "p1");
    task_init(&prod2, produtor, "p2");
    task_init(&prod3, produtor, "p3");
    task_init(&cons1, consumidor, "                  c1");
    task_init(&cons2, consumidor, "                  c2");
    task_wait(&prod1);
    return 0;
}

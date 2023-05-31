#include <stdio.h>
#include <stdlib.h>
#include "ppos.h"

#define BUFFER_SIZE 10  // Define Buffer Size
#define SLEEP_TIME 1000 // Define Sleep time

// Initialize variables for last insertion and consumption
int last_insertion = -1;
int last_consumed = -1;

int buffer[BUFFER_SIZE]; // Initialize buffer

// Initialize task variables and semaphore variables
task_t prod1, prod2, prod3, cons1, cons2;
semaphore_t s_buffer, s_item, s_vacancy;

// Producer function
void produtor(void *arg)
{
    int item;
    while (1)
    {
        task_sleep(SLEEP_TIME);            // Sleep for a defined time
        item = rand() % 100;               // Generate a random item between 0 and 100
        sem_down(&s_vacancy);              // Access vacancy semaphore
        sem_down(&s_buffer);               // Access buffer semaphore
        last_insertion++;                  // Increment last insertion
        if (last_insertion == BUFFER_SIZE) // Check if insertion has reached buffer limit
        {
            last_insertion = 0; // Reset last insertion
        }
        buffer[last_insertion] = item;                 // Add item to buffer
        printf("%s produced %d\n", (char *)arg, item); // Print production message
        sem_up(&s_buffer);                             // Release buffer semaphore
        sem_up(&s_item);                               // Release item semaphore
    }
}

// Consumer function
void consumidor(void *arg)
{
    int item;
    while (1)
    {
        sem_down(&s_item);                // Access item semaphore
        sem_down(&s_buffer);              // Access buffer semaphore
        last_consumed++;                  // Increment last consumed
        if (last_consumed == BUFFER_SIZE) // Check if consumption has reached buffer limit
        {
            last_consumed = 0; // Reset last consumed
        }
        item = buffer[last_consumed];                  // Get item from buffer
        printf("%s consumed %d\n", (char *)arg, item); // Print consumption message
        sem_up(&s_buffer);                             // Release buffer semaphore
        sem_up(&s_vacancy);                            // Release vacancy semaphore
    }
}

// Main function
int main()
{
    printf("main : início\n");
    ppos_init();                                           // Initialize ppos
    sem_init(&s_buffer, 1);                                // Initialize buffer semaphore
    sem_init(&s_item, 0);                                  // Initialize item semaphore
    sem_init(&s_vacancy, BUFFER_SIZE);                     // Initialize vacancy semaphore
    task_init(&prod1, produtor, "p1");                     // Create producer task 1
    task_init(&prod2, produtor, "p2");                     // Create producer task 2
    task_init(&prod3, produtor, "p3");                     // Create producer task 3
    task_init(&cons1, consumidor, "                  c1"); // Create consumer task 1
    task_init(&cons2, consumidor, "                  c2"); // Create consumer task 2
    task_wait(&prod1);                                     // Wait for producer task 1 to finish
    return 0;                                              // Return 0 upon successful execution
}

// GRR20197155 Gabriel Razzolini Pires De Paula

// Including libs needed
#include <stdio.h>
#include <stdlib.h>
#include "queue.h"

// Count the number of elements in the queue
int queue_size(queue_t *queue) 
{
    // If the queue pointer is NULL, return 0 (empty queue)
    if ( !queue ) 
        return 0;
    // Initialize size of the queue
    int size = 0; 
    // Initialize the current element of the queue by assigning it to the first element (queue->next)
    queue_t *current = queue->next; 
    // Count the first element (increment size by one)
    size++; 
    // Iterate through the queue until the current element reaches the first element
    while ( current && current != queue ) 
    {
        // Increment the size by one for each element in the queue
        size++; 
        // Move to the next element (current element now points to the next one)
        current = current->next; 
    }
    // Return the size of the queue
    return size; 
}

// Print the queue
void queue_print(char *name, queue_t *queue, void print_elem(void *)) 
{
    // Print the initial part of the output, formatted
    printf("%s: [", name); 
    // If the queue pointer is NULL
    if ( !queue ) 
    {
        // Print the closing bracket of the formatted output and a newline
        printf("]\n"); 
        // Return from the function
        return; 
    }
    // Initialize the current element of the queue by assigning it to queue->next
    queue_t *current = queue->next; 
    // Print the first element of the queue
    print_elem(queue); 
    // Iterate through the queue until the current element reaches the first element
    while ( current != queue ) 
    {
        // Print a space for formatting
        printf(" "); 
        // Print the current element
        print_elem(current); 
        // Move to the next element (current element now points to the next one)
        current = current->next; 
    }
    // Print the closing bracket of the formatted output and a newline
    printf("]\n"); 
}

// Adds a new element to the last position of the queue
int queue_append(queue_t **queue, queue_t *elem) 
{
    /****************************** ERROR CASES **********************************************/
    // If the element pointer is NULL
    if ( !elem ) 
    {
        // Print error message
        fprintf(stderr, "queue.c ERROR: Tried to insert a non-existing element into the queue!\n"); 
        // Return with error
        return -1; 
    }
    // If the element is already in a queue
    if ( (elem->next != NULL) || (elem->prev != NULL) ) 
    {
        // Print error message
        fprintf(stderr, "queue.c ERROR: Tried to insert an element that is already in a queue!\n"); 
        // Return with error
        return -2; 
    }
    // If the queue pointer is NULL
    if ( !queue ) 
    {
        // Print error message
        fprintf(stderr, "queue.c ERROR: Tried to insert element into a non-existing queue!\n"); 
        // Return with error
        return -3; 
    }
    /****************************** ERROR CASES **********************************************/
    // Check if the queue is empty
    if ( !(*queue) ) 
    {
        // Make the element point to itself in both directions
        elem->next = elem->prev = elem; 
        // Set the queue pointer to point to this new element
        *queue = elem; 
    }
    else // Queue is not empty
    {
        // Get the last element in the queue
        queue_t *last = (*queue)->prev; 
        // Make the last element point to the new element (the new element will become the last)
        last->next = elem; 
        // Make the new element's 'prev' attribute point to its previous: last (Last element is now the penultimate)
        elem->prev = last; 
        // Make the new element's 'next' attribute point to its next: first
        elem->next = *queue; 
        // Make the 'prev' attribute of the first element point to the new element (The new element is now the last one)
        (*queue)->prev = elem; 
    }
    // Return with success
    return 0; 
}

// Auxiliar function for queue_remove() that checks if two elements are in the same queue. PS: Assumes that both queues exist
// Returns 0 if they are in different queues
// Returns 1 if they are in the same queue
int in_same_queue(queue_t *first_queue, queue_t *second_queue) 
{
    // If the two elements are equal, they are in the same queue
    if ( first_queue == second_queue ) 
        return 1; // Return 1 to indicate they are in the same queue

    // Initialize a temporary element pointing to the next element of the first queue
    queue_t *temp = first_queue->next; 
    // Iterate through the first queue until the temporary element reaches the first element again
    while ( temp != first_queue ) 
    {
        // If the temporary element is equal to the second queue element, they are in the same queue
        if ( temp == second_queue ) 
            return 1; // Return 1 to indicate they are in the same queue
        // Move to the next element (the temporary element now points to the next one)
        temp = temp->next; 
    }
    // If the function has not returned yet
    return 0; // Return 0 to indicate the elements are in different queues
}

// Remove the 'elem' element from the queue
int queue_remove(queue_t **queue, queue_t *elem)
{
    /****************************** ERROR CASES **********************************************/
    // If the element pointer is NULL
    if ( !elem ) 
    {
        // Print error message
        fprintf(stderr, "queue.c ERROR: Tried to remove a non-existing element!\n"); 
        // Return with error
        return -1; 
    }
    // If the element is not in the specified queue
    if ( !( in_same_queue(*queue, elem) ) ) 
    {
        // Print error message
        fprintf(stderr, "queue.c ERROR: Tried to remove an element from a queue that does not contain it!\n"); 
        // Return with error
        return -2; 
    }
    // If the queue pointer is NULL
    if ( !queue ) 
    {
        // Print error message
        fprintf(stderr, "queue.c ERROR: Tried to remove element from a non-existing queue!\n"); 
        // Return with error
        return -3; 
    }
    // If the queue is empty
    if ( !(*queue) ) 
    {
        // Print error message
        fprintf(stderr, "queue.c ERROR: Tried to remove an element from an empty queue!\n"); 
        // Return with error
        return -4; 
    }    
    /****************************** ERROR CASES **********************************************/
    // If the queue has only one element
    if ( elem->next == elem ) 
    {
        // Remove the element's next and prev pointers, setting them to NULL
        elem->next = elem->prev = NULL; 
        // Set the queue pointer to NULL, indicating that the queue is now empty
        *queue = NULL; 
        // Return with success
        return 0; 
    }

    // If the element is the first in the queue then set the queue pointer to the next element
    if ( *queue == elem ) 
        *queue = (*queue)->next; 

    // Get the next element in the queue
    queue_t *next_in_queue = elem->next; 
    // Get the previous element in the queue
    queue_t *previous_in_queue = elem->prev; 
    // Update the 'next' attribute of the previous element to remove 'elem'
    previous_in_queue->next = next_in_queue; 
    // Update the 'prev' attribute of the next element to remove 'elem'
    next_in_queue->prev = previous_in_queue; 
    // Set the 'next' and 'prev' attributes of the element to NULL, removing it from the queue
    elem->next = elem->prev = NULL; 
    // Return with success
    return 0; 
}

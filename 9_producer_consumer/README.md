# Producer/Consumer System with Bounded Buffer

This project involves utilizing my system, implemented with semaphore functions from the previous project(**`8_semaphore`**), to construct a producer/consumer system with a limited buffer.

Here's the basic code structure for the producer/consumer system:

## Producer
```
void producer()
{
   while (true)
   {
      task_sleep (1000);
      item = random (0..99);

      sem_down(&s_vaga);

      sem_down(&s_buffer);
      // insert item into the buffer
      sem_up(&s_buffer);

      sem_up(&s_item);
   }
}
```

## Consumer
```
void consumer()
{
   while (true)
   {
      sem_down(&s_item);

      sem_down(&s_buffer);
      // remove item from the buffer
      sem_up(&s_buffer);

      sem_up(&s_vaga);

      // print item
      task_sleep (1000);
   }
}
```
## Key Notes
- I've created a file named **`producer-consumer.c`**, in which i've defined the producer, consumer, and main tasks.
- The main variables implemented in this project include:
    - **`item`**: An integer value between 0 and 99.
    - **`buffer`**: A queue of integers with capacity for up to 5 elements, initially empty, accessed using a FIFO policy. This were implemented using an integer array.
    - **`s_buffer`**, **`s_item`**, **`s_vaga`**: Semaphores, properly initialized.
- The implemented have 3 producers and 2 consumers. The output look something like this(but, feel free to try it on your own):

```
p1 produced 37
p2 produced 11
                             c1 consumed 37
p3 produced 64
p1 produced 21
                             c2 consumed 11
p2 produced 4
                             c1 consumed 64
...                          ...
```

Please note, the numbers are consumed in the sequence they were produced, indicating the buffer's FIFO behavior.
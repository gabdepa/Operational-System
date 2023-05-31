# Producer/Consumer System with Bounded Buffer

This project involves utilizing my system, implemented with semaphore functions from the previous project(**`8_semaphore`**), to construct a producer/consumer system with a limited buffer. The image below ilustrate this:

![Schema](https://wiki.inf.ufpr.br/maziero/lib/exe/fetch.php?cache=&media=so:coord-prodcons.png)

Here's the basic code structure for the producer/consumer system:
## Producer
```
void producer()
{
   while (true)
   {
      task_sleep (1000);
      item = random (0..99);

      sem_down(&s_vacancy);

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

      sem_up(&s_vacancy);

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
    - **`s_buffer`**, **`s_item`**, **`s_vacancy`**: Semaphores, properly initialized.
- The code implemented have 2 producers and 3 consumers. The output look something like this(but, feel free to try it!):

```
...                          ...
p1 produced 7
p2 produced 88
                  c1 consumed 7
                  c2 consumed 88
p1 produced 29
p2 produced 8
                  c3 consumed 29
                  c2 consumed 8
...                          ...
```

Please note, the numbers are consumed in the sequence they were produced, indicating the buffer's FIFO behavior.

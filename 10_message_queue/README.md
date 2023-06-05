# Message Queue

A message queue is a structure used by tasks for communication through the exchange of fixed-size messages. Each queue can store up to **`N`** fixed-size messages, respecting a FIFO access policy. The below image describes the proccess:

![Schema](https://wiki.inf.ufpr.br/maziero/lib/exe/fetch.php?cache=&media=so:prod-soma-cons.png)

Access to the queue is blocking. This means that a task attempting to send a message to a full queue must wait until spaces are available in the queue. Similarly, a task wishing to receive messages from an empty queue must wait until a message is available in that queue.

## Queue Initialization

`int mqueue_init(mqueue_t *queue, int max_msgs, int msg_size)`

This function initializes the message queue pointed to by **`queue`**, with capacity to receive up to **`max_msgs`** messages, each msg_size bytes in size, and initially empty. Returns 0 in case of success and -1 in case of error.

## Send a Message to the Queue

`int mqueue_send(mqueue_t *queue, void *msg)`

This function sends the message pointed to by **`msg`** to the end of the **`queue`**. This call is blocking: if the queue is full, the current task is suspended until the send can be done. The msg pointer points to a buffer containing the message to send, which should be copied into the queue. Returns 0 in case of success and -1 in case of error.

## Receive a Message from the Queue

`int mqueue_recv(mqueue_t *queue, void *msg)`

This function receives a message from the start of the **`queue`** and deposits it in the buffer pointed to by **`msg`**. This call is blocking: if the queue is empty, the current task is suspended until the reception can be done. The **`msg`** pointer points to a buffer that will receive the message. Returns 0 in case of success and -1 in case of error.

## Queue Termination

`int mqueue_destroy(mqueue_t *queue)`

This function terminates the message queue indicated by **`queue`**, destroying its content and releasing all tasks waiting for messages from it (these tasks should return from their respective calls with a return value of -1). Returns 0 in case of success and -1 in case of error.

## Number of Messages in the Queue

`int mqueue_msgs(mqueue_t *queue)`

This function informs the number of messages present in the queue indicated by **`queue`**. Returns 0 or **`+N`** in case of success and -1 in case of error.

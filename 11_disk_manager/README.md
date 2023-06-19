# Disk Manager

This project aims to implement input/output operations (reading and writing) of data blocks on a virtual hard drive. The execution of these operations will be handled by a disk manager, which serves as the disk access driver. Down below there is a image describing the structure of the code:

![Schema](https://wiki.inf.ufpr.br/maziero/lib/exe/fetch.php?cache=&media=so:ppos_disk.png)

## Disk Access Interface

The main task initializes the disk manager/driver through the following call:

**`int disk_mgr_init (&num_blocks, &block_size);`**

Upon returning from the call, the variable **`num_blocks`** contains the number of blocks of the initialized disk, while the variable **`block_size`** contains the size of each disk block, in bytes. This call returns 0 in case of success or -1 in case of error.

Tasks can read and write data blocks on the virtual disk through the following calls (both blocking):

**`int disk_block_read  (int block, void* buffer) ;`**
**`int disk_block_write (int block, void* buffer) ;`**

 - **`block`**: position (block number) to read or write on the disk (must be between 0 and numblocks-1);
 - **`buffer`**: address of the data to be written to the disk, or where the data read from the disk should be placed; this buffer must have capacity for block_size bytes.
 - return: 0 in case of success or -1 in case of error.

Each task that requests a read/write operation on the disk should be suspended until the requested operation is completed. Suspended tasks should be in a wait queue associated with the disk. The read/write requests present in this queue should be serviced in the order they were generated, according to the FCFS (First Come, First Served) disk scheduling policy.

## The Virtual Disk

The "virtual disk" simulates the logical and temporal behavior of a real hard disk, with the following characteristics:

- The content of the virtual disk is mapped to a UNIX file in the current directory of the real machine, named default **`disk.dat`**. The content of the virtual disk is maintained from one execution to another.
- The disk contains N blocks of the same size. The number of disk blocks will depend on the size of the underlying file in the real system.
- As on a real disk, read/write operations are always done one block at a time. It is not possible to read or write isolated bytes, part of a block, or several blocks at the same time.
- Read/write requests made to the disk are asynchronous, i.e., they only register the requested operation, without blocking the call. The completion of each read/write operation is later indicated by the disk through a UNIX SIGUSR1 signal, which must be captured and handled.
- The disk only handles one read/write at a time. While the disk is handling a request, it is in a busy state; attempts to access a busy disk return an error code.
- The disk's response time is proportional to the distance between the current position of the disk's read head and the position of the requested operation. Initially, the read head is positioned over the initial block (zero).

The code that simulates the disk is in disk.c and its access interface is defined in disk.h; these files should not be modified. Disk access should be done only through the definitions present in disk.h. The definitions present in disk.c implement (simulate) the internal behavior of the disk and therefore should not be used in your code.

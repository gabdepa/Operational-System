#ifndef __DISK_MGR__
#define __DISK_MGR__

#include "ppos_data.h"

typedef enum
{
	READ,
	WRITE
} operations;

typedef struct request_type
{
	struct request_type *next, *prev;
	task_t *task;
	operations operation;
	int block;
	void *buffer;
} request_type;

// estrutura que representa um disco no sistema operacional

typedef struct
{
	request_type *request;
	request_type *requests;
	task_t *queue;
	semaphore_t semaphore;
} disk_t;

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init(int *numBlocks, int *blockSize);

// leitura de um bloco, do disco para o buffer
int disk_block_read(int block, void *buffer);

// escrita de um bloco, do buffer para o disco
int disk_block_write(int block, void *buffer);

#endif
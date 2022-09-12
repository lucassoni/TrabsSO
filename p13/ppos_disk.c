// GRR20190395 Lucas Soni Teixeira
#include "ppos_disk.h"
#include "disk.h"
#include "ppos.h"
#include "ppos_data.h"
#include "globalvars.h"

#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

struct sigaction action;

int disk_signal;

disk_t disk;

task_t disk_manager;
task_t *taskQueue;
task_t *curTask;
task_t mainTask;

void disk_signal_handler()
{
    disk_signal = 1;
    if (disk_manager.status == SUSPENSA)
    {
        queue_append((queue_t **)&taskQueue, (queue_t *)&disk_manager);
        disk_manager.status = PRONTA;
    }
}

static void suspend_task(task_t *task)
{
    queue_remove((queue_t **)&taskQueue, (queue_t *)task);
    queue_append((queue_t **)&disk.task_queue, (queue_t *)task);
    task->status = SUSPENSA;
}

// Reativa a tarefa task e a coloca na fila de prontas
static void enable_task(task_t *task)
{
    queue_remove((queue_t **)&disk.task_queue, (queue_t *)task);
    queue_append((queue_t **)&taskQueue, (queue_t *)task);
    task->status = PRONTA;
}

static request_t *create_request(int block, void *buffer, int request_type)
{
    request_t *req = malloc(sizeof(request_t));
    if (req == NULL)
    {
        return 0;
    }

    req->next = NULL;
    req->prev = NULL;
    req->task = curTask;
    req->block = block;
    req->buffer = buffer;
    req->type = request_type;

    return req;
}

void diskDriverBody(void *args)
{
    while (1)
    {
        // obtém o semáforo de acesso ao disco
        sem_down(&disk.sem_disk);
        // se foi acordado devido a um sinal do disco
        if (disk_signal)
        {
            task_t *task = disk.request_queue->task;
            request_t *req = disk.request_queue;

            // acorda a tarefa cujo pedido foi atendido
            enable_task(task);
            // remove o pedido da fila de pedidos
            queue_remove((queue_t **)&disk.request_queue, (queue_t *)req);
            free(req);

            disk_signal = 0;
        }

        // se o disco estiver livre e houver pedidos de E/S na fila
        if ((disk_cmd(DISK_CMD_STATUS, 0, 0) == 1) && (disk.request_queue != NULL))
        {
            // escolhe na fila o pedido a ser atendido, usando FCFS
            request_t *req = disk.request_queue;

            // solicita ao disco a operação de E/S, usando disk_cmd()
            if (req->type == READ)
            {
                if (disk_cmd(DISK_CMD_READ, req->block, req->buffer))
                {
#ifdef DEBUG
                    printf("Erro ao ler disco\n");
#endif
                }
            }
            else if (req->type == WRITE)
            {
                if (disk_cmd(DISK_CMD_WRITE, req->block, req->buffer))
                {
#ifdef DEBUG
                    printf("Erro ao escrever no disco\n");
#endif
                }
            }
            else
            {
#ifdef DEBUG
                printf("Operacao de disco invalida\n");
#endif
            }
        }

        // libera o semáforo de acesso ao disco
        sem_up(&disk.sem_disk);
        // suspende a tarefa corrente (retorna ao dispatcher)
        queue_remove((queue_t **)&taskQueue, (queue_t *)&disk_manager);
        disk_manager.status = SUSPENSA;

        task_yield();
    }
}

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init(int *numBlocks, int *blockSize)
{
    if (disk_cmd(DISK_CMD_INIT, 0, 0))
    {
        return -1;
    }

    *numBlocks = disk_cmd(DISK_CMD_DISKSIZE, 0, 0);
    if (*numBlocks < 0)
    {
        return -1;
    }

    *blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0);
    if (*blockSize < 0)
    {
        return -1;
    }

    sem_create(&disk.sem_disk, 1);

    disk_signal = 0;

    task_create(&disk_manager, diskDriverBody, NULL);

    disk_manager.status = SUSPENSA;

    action.sa_handler = disk_signal_handler;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGUSR1, &action, 0) < 0)
    {
        perror("Erro em sigaction: ");
        exit(1);
    }

    return 0;
}

// leitura de um bloco, do disco para o buffer
int disk_block_read(int block, void *buffer)
{
    // obtém o semáforo de acesso ao disco
    sem_down(&disk.sem_disk);

    // inclui o pedido na fila_disco
    request_t *req = create_request(block, buffer, READ);
    if (req == NULL)
    {
        return -1;
    }

    queue_append((queue_t **)&disk.request_queue, (queue_t *)req);

    if (disk_manager.status == SUSPENSA)
    {
        queue_append((queue_t **)&taskQueue, (queue_t *)&disk_manager);
        disk_manager.status = PRONTA;
        // acorda o gerente de disco (põe ele na fila de prontas)
    }

    // libera semáforo de acesso ao disco
    sem_up(&disk.sem_disk);
    // suspende a tarefa corrente (retorna ao dispatcher)
    suspend_task(curTask);
    task_yield();

    return 0;
}

// escrita de um bloco, do buffer para o disco
int disk_block_write(int block, void *buffer)
{
    // obtém o semáforo de acesso ao disco
    sem_down(&disk.sem_disk);

    // inclui o pedido na fila_disco
    request_t *req = create_request(block, buffer, WRITE);
    if (req == NULL)
    {
        return -1;
    }

    queue_append((queue_t **)&disk.request_queue, (queue_t *)req);

    if (disk_manager.status == SUSPENSA)
    {
        queue_append((queue_t **)&taskQueue, (queue_t *)&disk_manager);
        disk_manager.status = PRONTA;
        // acorda o gerente de disco (põe ele na fila de prontas)
    }

    // libera semáforo de acesso ao disco
    sem_up(&disk.sem_disk);
    // suspende a tarefa corrente (retorna ao dispatcher)
    suspend_task(curTask);
    task_yield();

    return 0;
}
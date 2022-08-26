#include "ppos_disk.h"
#include "disk.h"
#include "ppos.h"
#include "globalvars.h"

#include <stdlib.h>
#include <signal.h>
#include <stdio.h>

struct sigaction action;

int disk_signal;

disk_t disk;

void diskDriverBody (void * args)
{
   while (1) 
   {
        // obtém o semáforo de acesso ao disco
        sem_down(&disk.sem_disk);
        // se foi acordado devido a um sinal do disco
        if (disk_signal)
        {
            // acorda a tarefa cujo pedido foi atendido
        }
 
        // se o disco estiver livre e houver pedidos de E/S na fila
        if (disk.task_queue && (disk.request_queue != NULL))
        {
            // escolhe na fila o pedido a ser atendido, usando FCFS
            // solicita ao disco a operação de E/S, usando disk_cmd()
        }
    
        // libera o semáforo de acesso ao disco
        sem_up(&disk.sem_disk);
        // suspende a tarefa corrente (retorna ao dispatcher)
   }
}

// inicializacao do gerente de disco
// retorna -1 em erro ou 0 em sucesso
// numBlocks: tamanho do disco, em blocos
// blockSize: tamanho de cada bloco do disco, em bytes
int disk_mgr_init (int *numBlocks, int *blockSize) {

    if (disk_cmd(DISK_CMD_INIT, 0, 0)) // Se o disco na ofoi corretamente inicializado
        return -1;
    *numBlocks = (disk_cmd(DISK_CMD_DISKSIZE, 0, 0))
    if (numBlocks < 0) // Se numblocks e valido
        return -1;
    if ((*blockSize = disk_cmd(DISK_CMD_BLOCKSIZE, 0, 0)) < 0) // Se blockSize e valido
        return -1;

    return 0;
}

// leitura de um bloco, do disco para o buffer
int disk_block_read (int block, void *buffer) {
    // obtém o semáforo de acesso ao disco
    sem_down(&disk.sem_disk);
    // inclui o pedido na fila_disco
    
    if (gerente de disco está dormindo)
    {
        // acorda o gerente de disco (põe ele na fila de prontas)
    }
    
    // libera semáforo de acesso ao disco
    sem_up(&disk.sem_disk);
    // suspende a tarefa corrente (retorna ao dispatcher)
}

// escrita de um bloco, do buffer para o disco
int disk_block_write (int block, void *buffer) {
    return 1;
}
// GRR20190395 Lucas Soni Teixeira
// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// Estruturas de dados internas do sistema operacional

#ifndef __PPOS_DATA__
#define __PPOS_DATA__

#include <ucontext.h>		// biblioteca POSIX de trocas de contexto
#include "queue.h"

#define PRONTA 0
#define TERMINADA 1
#define SUSPENSA 2
#define DORMINDO 3


// Estrutura que define um Task Control Block (TCB)
typedef struct task_t
{
  struct task_t *prev, *next;		// ponteiros para usar em filas
  int id;				// identificador da tarefa
  ucontext_t context;			// contexto armazenado da tarefa
  short status;			// pronta, rodando, suspensa, ...
  short preemptable;			// pode ser preemptada?
  int pDinamica, pEstatica; // prioridades dinamicas e estaticas
  unsigned int execTime, processTime; // tempo de execucao e processamento
  unsigned int activations; // numero de ativacoes
  int exitCode; // codigo de fim de programa
  queue_t *suspendedQueue; // fila de tasks que essa tarefa  suspendeu
  unsigned int sleepTime; // tempo que a tarefa deve acordar
   // ... (outros campos serão adicionados mais tarde)
} task_t;

// estrutura que define um semáforo
typedef struct
{
  queue_t *queue; // fila de tasks esperando o semaforo
  int counter; // contador do semaforo
  int lock; // indicador de validez do semaforo
  // preencher quando necessário
} semaphore_t ;

// estrutura que define um mutex
typedef struct
{
  // preencher quando necessário
} mutex_t ;

// estrutura que define uma barreira
typedef struct
{
  // preencher quando necessário
} barrier_t ;

// estrutura que define uma fila de mensagens
typedef struct
{
  // preencher quando necessário
} mqueue_t ;

#endif


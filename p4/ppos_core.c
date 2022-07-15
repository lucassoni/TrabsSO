#include "ppos.h"
#include <stdlib.h>
#include <stdio.h>
#include "queue.h"

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

#define MAX_PRIORITY -20
#define MIN_PRIORITY 20
#define TA_ALPHA 1

int taskID, quantTasks;
task_t *curTask, *prevTask, mainTask, dispatcherTask, *taskQueue = NULL;

static void dispatcher();

task_t *scheduler() {
    task_t *p, *menor;

    p = taskQueue;
    menor = p;
    p = p->next;

    while (p != taskQueue) {
        if (p->pDinamica < menor->pDinamica) {
            menor->pDinamica -= TA_ALPHA;
            menor = p;

        } 
        else p->pDinamica -= TA_ALPHA;
        p = p->next;    
    }
    menor->pDinamica = menor->pEstatica;

    #ifdef DEBUG
        printf("Scheduler returned prio task\n");
    #endif
    return menor;
}

void ppos_init() {
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf(stdout, 0, _IONBF, 0);

    taskID = 0;

    mainTask.prev = NULL;
    mainTask.next = NULL;
    mainTask.id = taskID;
    getcontext(&(mainTask.context));

    curTask = &mainTask;

    quantTasks = 0;

    task_create(&dispatcherTask, dispatcher, NULL);

    #ifdef DEBUG
        printf("PPOS: system initialized\n");
    #endif
}

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create(task_t *task, void (*start_func)(void *), void *arg) {
    
    if (!task) {
        return -1;
    }

    char *stack = malloc(STACKSIZE);
    if (!stack) {
        perror("Erro na criação da pilha\n");
        exit(1);
    }

    getcontext(&(task->context));

    task->context.uc_stack.ss_sp = stack;
    task->context.uc_stack.ss_size = STACKSIZE;
    task->context.uc_stack.ss_flags = 0;
    task->context.uc_link = 0;

    makecontext(&(task->context), (void*)(*start_func), 1, arg);

    task->prev = NULL;
    task->next = NULL;
    taskID++;
    task->id = taskID;
    task->status = PRONTA;
    task->pEstatica = 0;
    task->pDinamica = 0;

    if (taskID > 1) {
        queue_append((queue_t**) &taskQueue, (queue_t *) task);
        quantTasks++;
    }
    
    #ifdef DEBUG
        printf("PPOS: task %d created by task %d\n", task->id, curTask->id);
    #endif

    return taskID;   
}

// Termina a tarefa corrente, indicando um valor de status encerramento
void task_exit(int exit_code) {
    #ifdef DEBUG
        printf("PPOS: task %d ended with exit code %d\n", curTask->id, exit_code);
    #endif
    curTask->status = TERMINADA;
    if (curTask->id == 1) {
        free(dispatcherTask.context.uc_stack.ss_sp);
        task_switch(&mainTask);
    }
    task_switch(&dispatcherTask);
}

// alterna a execução para a tarefa indicada
int task_switch(task_t *task) {
    if (!task) {
        return -1;
    }

    #ifdef DEBUG
        printf("PPOS: switch task %d -> task %d\n", curTask->id, task->id) ;
    #endif

    prevTask = curTask;
    curTask = task;

    swapcontext(&(prevTask->context), &(task->context));

    return 0;
}

// retorna o identificador da tarefa corrente (main deve ser 0)
int task_id() {
    return curTask->id;
}

static void dispatcher() {
    while (quantTasks > 0) {
        task_t *next = scheduler();
        if (!next) {
            fprintf(stderr, "erro task nula\n");
            exit (0);
        }

        task_switch(next);

        if (next->status == TERMINADA) {
            queue_remove((queue_t **) &taskQueue, (queue_t*) next);
            free(next->context.uc_stack.ss_sp);
            quantTasks--;
        }

        #ifdef DEBUG
            printf("Dispatcher") ;
        #endif
    }
    task_exit(0);
}

void task_yield() {
    task_switch(&dispatcherTask);
}

void task_setprio (task_t *task, int prio) {
    if (!task) {
        if (prio < MIN_PRIORITY && prio > MAX_PRIORITY) {    
            curTask->pEstatica = prio;
            curTask->pDinamica = prio;
        }
        else {
            fprintf(stderr, "prioridade invalida\n");
            exit(0);
        }
    }
    else if (prio < MIN_PRIORITY && prio > MAX_PRIORITY) {    
       task->pEstatica = prio;
       task->pDinamica = prio;
    }
    else {
        fprintf(stderr, "prioridade invalida\n");
        exit(0);
    }
}

int task_getprio (task_t *task) {
    if (!task) {
        return curTask->pEstatica;
    }
    return task->pEstatica;
}
#include "ppos.h"
#include <stdlib.h>
#include <stdio.h>

#define DEBUG

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

int taskID;
task_t *curTask, *prevTask, mainTask;



void ppos_init() {
    /* desativa o buffer da saida padrao (stdout), usado pela função printf */
    setvbuf(stdout, 0, _IONBF, 0);

    taskID = 0;

    mainTask.prev = NULL;
    mainTask.next = NULL;
    mainTask.id = taskID;
    getcontext(&(mainTask.context));

    curTask = &mainTask;

    #ifdef DEBUG
    printf("PPOS: system initialized");
    #endif
}

// Cria uma nova tarefa. Retorna um ID> 0 ou erro.
int task_create(task_t *task, void (*start_func)(void *), void *arg) {
    
    if (!task) {
        return -1;
    }

    char *stack = malloc(STACKSIZE);
    if (!stack) {
        perror("Erro na criação da pilha: ");
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
    task_switch(&mainTask);
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
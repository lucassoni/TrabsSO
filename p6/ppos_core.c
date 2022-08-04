#include "ppos.h"
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <sys/time.h>

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

#define MAX_PRIORITY -20
#define MIN_PRIORITY 20
#define TA_ALPHA 1
#define QUANTUM 20
// #define DEBUG

int taskID, quantTasks, temporizador, relogio, relogioTask;
task_t *curTask, *prevTask, mainTask, dispatcherTask, *taskQueue = NULL;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action ;

// estrutura de inicialização to timer
struct itimerval timer;

static void tratador();
static void dispatcher();
static void set_timer();

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

    temporizador = QUANTUM;
    relogio = 0;
    relogioTask = 0;

    set_timer();

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
    task->processTime = systime();
    task->execTime = 0;
    task->activations = 0;

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

    curTask->processTime = systime() - curTask->processTime;

    if (curTask->id == 1) {
        printf("Dispatcher Task exit: execution time %d ms, processor time %d ms, %d activations\n", dispatcherTask.execTime, dispatcherTask.processTime, dispatcherTask.activations);
        free(dispatcherTask.context.uc_stack.ss_sp);
        task_switch(&mainTask);
    }

    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", curTask->id, curTask->execTime, curTask->processTime, curTask->activations);
    task_switch(&dispatcherTask);
}

// alterna a execução para a tarefa indicada
int task_switch(task_t *task) {
    if (!task) {
        return -1;
    }

    #ifdef DEBUG
        if (task == &dispatcherTask) {
            printf("\nPPOS: switch task %d -> dispatcher task\n\n", curTask->id);
        }
        else if (curTask == &dispatcherTask) {
            printf("\nPPOS: switch dispatcher task -> task %d\n\n", task->id); 
        }
        else printf("\nPPOS: switch task %d -> task %d\n\n", curTask->id, task->id); 
    #endif

    curTask->execTime += (systime() - relogioTask);
    prevTask = curTask;
    curTask = task;

    temporizador = QUANTUM;
    task->activations++;
    relogioTask = systime();
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
            printf("Dispatcher\n") ;
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

static void set_timer() {
    // registra a ação para o sinal de timer SIGALRM
    action.sa_handler = tratador;
    sigemptyset(&action.sa_mask);
    action.sa_flags = 0;
    if (sigaction(SIGALRM, &action, 0) < 0)
    {
        perror("Erro em sigaction: ");
        exit(1);
    }

    // ajusta valores do temporizador
    timer.it_value.tv_usec = 1000;      // primeiro disparo, em micro-segundos
    timer.it_value.tv_sec  = 0;      // primeiro disparo, em segundos
    timer.it_interval.tv_usec = 1000;   // disparos subsequentes, em micro-segundos
    timer.it_interval.tv_sec  = 0;   // disparos subsequentes, em segundos

    // arma o temporizador ITIMER_REAL (vide man setitimer)
    if (setitimer(ITIMER_REAL, &timer, 0) < 0)
    {
        perror("Erro em setitimer: ");
        exit (1);
    }
}

static void tratador() {
    if (curTask == &dispatcherTask) {
        return;
    }

    relogio++;
    temporizador--;
    if (temporizador == 0) {
        task_yield();
    }

    #ifdef DEBUG
        printf("QUANTUM: %d\n", temporizador);
    #endif
}

unsigned int systime() {
    return relogio;
}
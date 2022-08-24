// GRR20190395 Lucas Soni Teixeira
#include "ppos.h"
#include "queue.h"
#include <stdlib.h>
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <sys/time.h>

#define STACKSIZE 64*1024	/* tamanho de pilha das threads */

#define MAX_PRIORITY -20
#define MIN_PRIORITY 20
#define TA_ALPHA 1
#define QUANTUM 10
// #define DEBUG

int taskID, quantTasks, quantSleepTasks, temporizador, relogio, relogioTask;
task_t *curTask, *prevTask, mainTask, dispatcherTask, *sleepQueue = NULL, *taskQueue = NULL;

// estrutura que define um tratador de sinal (deve ser global ou static)
struct sigaction action ;

// estrutura de inicialização to timer
struct itimerval timer;

static void tratador();
static void dispatcher();
static void set_timer();
static void wake_tasks();

task_t *scheduler() {
    task_t *p, *menor;

    p = taskQueue;
    if (p == NULL) {
        return NULL;
    }
    
    
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
    mainTask.pDinamica = 0;
    mainTask.pEstatica = 0;
    mainTask.execTime = systime();
    mainTask.processTime = 0;
    mainTask.activations = 0;
    mainTask.suspendedQueue = NULL;
    getcontext(&(mainTask.context));

    curTask = &mainTask;

    quantTasks = 0;
    quantSleepTasks = 0;

    queue_append((queue_t**) &taskQueue, (queue_t *) &mainTask);
    quantTasks++;

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
    task->processTime = 0;
    task->execTime = systime();
    task->activations = 0;
    task->suspendedQueue = NULL;

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

    curTask->execTime = systime() - curTask->execTime;
    curTask->processTime += (systime() - relogioTask);

    curTask->exitCode = exit_code;

    queue_t *aux = curTask->suspendedQueue;
    while(aux != NULL) {
        task_resume((task_t *) aux, (task_t **) &curTask->suspendedQueue);
        aux = curTask->suspendedQueue;
    }

    printf("Task %d exit: execution time %d ms, processor time %d ms, %d activations\n", curTask->id, curTask->execTime, curTask->processTime, curTask->activations);

    if (curTask->id == 1) {
        free(dispatcherTask.context.uc_stack.ss_sp);
        task_switch(&mainTask);
    }
    else task_switch(&dispatcherTask);
}

// alterna a execução para a tarefa indicada
int task_switch(task_t *task) {
    if (!task) {
        return -1;
    }

    #ifdef DEBUG
        printf("\nPPOS: switch task %d -> task %d\n\n", curTask->id, task->id); 
    #endif

    curTask->processTime += (systime() - relogioTask);
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
    while (quantTasks > 0 || queue_size((queue_t *) sleepQueue)) {
        wake_tasks();
        task_t *next = scheduler();

        if (next != NULL) {
            task_switch(next);

            if (next->status == TERMINADA) {
                queue_remove((queue_t **) &taskQueue, (queue_t*) next);
                free(next->context.uc_stack.ss_sp);
                quantTasks--;
            }
        }
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
    relogio++;
    if (curTask == &dispatcherTask) {
        return;
    }

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


void task_suspend (task_t **queue) {
    queue_remove((queue_t **) &taskQueue, (queue_t*) curTask);
    curTask->status = SUSPENSA;
    queue_append((queue_t **) queue, (queue_t*) curTask);
    task_yield();
}

void task_resume (task_t * task, task_t **queue) {
    if (queue != NULL) {
        queue_remove((queue_t **) queue, (queue_t*) task);
        task->status = PRONTA;
        queue_append((queue_t**) &taskQueue, (queue_t *) task);
    }
}

int task_join(task_t *task) {
    if (task == NULL || task->status == TERMINADA)
        return -1;
        
    task_suspend((task_t **) &task->suspendedQueue);
    return task->exitCode;
}

void task_sleep (int t) {
    curTask->sleepTime = systime() + t;
    queue_remove((queue_t **) &taskQueue, (queue_t*) curTask);
    curTask->status = DORMINDO;
    queue_append((queue_t**) &sleepQueue, (queue_t *) curTask);

    #ifdef DEBUG
        printf("Tem %d tasks dormindo\n", queue_size((queue_t *) sleepQueue));
        printf("Sleeping task %d with wake time %d\n", curTask->id, curTask->sleepTime);
    #endif
    task_yield();
}

void wake_tasks() {
    task_t *p = sleepQueue;
    int n = queue_size((queue_t *) sleepQueue);
    for (int i = 0; i < n; i++) {
        if (p->sleepTime <= systime()) {
            task_t *aux = p;
            queue_remove((queue_t **) &sleepQueue, (queue_t *) p);
            p->status = PRONTA;
            queue_append((queue_t**) &taskQueue, (queue_t *) p);

            #ifdef DEBUG
                printf("Waking task %d with wake time %d at time %d\n", p->id, p->sleepTime, systime());
            #endif

            p = aux;
        }
        p = p->next;
    }
}

void enter_cs (int *lock)
{
  // atomic OR (Intel macro for GCC)
  while (__sync_fetch_and_or (lock, 1)) ;   // busy waiting
}
 
void leave_cs (int *lock)
{
  (*lock) = 0 ;
}

int sem_create (semaphore_t *s, int value) {
    if (s == NULL) {
        return -1;
    }

    s->lock = 0;
    s->counter = value;   
    s->queue = NULL;

    return 0;
}

int sem_destroy (semaphore_t *s) {
    if (s == NULL) {
        return -1;
    }

    while (queue_size(s->queue) > 0) {
        task_t *task = (task_t *) s->queue;
        task_resume(task, (task_t **) &s->queue);
    }
    s = NULL;

    return 0;
}

int sem_down (semaphore_t *s) {
    if (s == NULL) {
        return -1;
    }

    enter_cs(&s->lock);
    s->counter--;
    leave_cs(&s->lock);

    if (s->counter < 0) {
        task_suspend((task_t **) &s->queue);
    }

    return 0;    
}

int sem_up (semaphore_t *s) {
    if (s == NULL) {
        return -1;
    }

    enter_cs(&s->lock);
    s->counter++;
    leave_cs(&s->lock);

    if (queue_size(s->queue) > 0) {
        task_t *task = (task_t *) s->queue;
        task_resume(task, (task_t **) &s->queue);
    }

    return 0;
}

int mqueue_create (mqueue_t *queue, int max_msgs, int msg_size) {
    if (queue == NULL) {
        return -1;
    }
    queue->max_msgs = max_msgs;
    queue->msg_size = msg_size;
    sem_create(&queue->s_buffer, 1);
    sem_create(&queue->s_item, 0);
    sem_create(&queue->s_vaga, max_msgs);
    
    queue->buffer = malloc(max_msgs * msg_size);
    queue->buffer_start = 0;
    queue->buffer_count = 0;
    
    return 0;
}

void *queue_position(mqueue_t *queue, int pos) {
    return (queue->buffer + (((queue->buffer_start + queue->buffer_count) % (queue->max_msgs)) + pos) * queue->msg_size);
}

int mqueue_send (mqueue_t *queue, void *msg) {
    if (queue == NULL || msg == NULL) {
        return -1;
    }

    if (sem_down(&queue->s_vaga) || sem_up(&queue->s_buffer)) {
        return -1;
    }

    memcpy(queue_position(queue->buffer, 0), msg, queue->msg_size);
    queue->buffer_count++;

    if (sem_up(&queue->s_buffer) || sem_up(&queue->s_item)) {
        return -1;
    }

    return 0;
}

int mqueue_recv (mqueue_t *queue, void *msg) {
    if (queue == NULL || msg == NULL) {
        return -1;
    }

    if (sem_down(&queue->s_item) || sem_down(&queue->s_item)) {
        return -1;
    }

    memcpy(msg, queue_position(queue->buffer, 0), queue->msg_size);
    queue->buffer_count--;
    queue->buffer_start++;

    if (sem_up(&queue->s_buffer) || sem_up(&queue->s_vaga)) {
        return -1;
    }

    return 0;
}

int mqueue_destroy (mqueue_t *queue) {
    if (queue == NULL) {
        return -1;
    }
    
    sem_destroy(&queue->s_buffer);
    sem_destroy(&queue->s_item);
    sem_destroy(&queue->s_vaga);
    free(queue->buffer);

    return 0;
}

int mqueue_msgs (mqueue_t *queue) {
    if (queue == NULL) {
        return -1;
    }

    return queue->buffer_count;
}
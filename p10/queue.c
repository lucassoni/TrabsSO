// GRR20190395 Lucas Soni Teixeira
#include "queue.h"
#include <stdio.h>
//------------------------------------------------------------------------------
// Conta o numero de elementos na fila
// Retorno: numero de elementos na fila

int queue_size (queue_t *queue) {

    if (queue != NULL) {
        queue_t *p = queue->next;
        int count = 1;  
        while (p != queue) {
            count++;
            p = p->next;
        }
        return count;
    }
    return 0;
}

//------------------------------------------------------------------------------
// Percorre a fila e imprime na tela seu conteúdo. A impressão de cada
// elemento é feita por uma função externa, definida pelo programa que
// usa a biblioteca. Essa função deve ter o seguinte protótipo:
//
// void print_elem (void *ptr) ; // ptr aponta para o elemento a imprimir
void queue_print (char *name, queue_t *queue, void print_elem (void*) ) {
    printf("%s", name);
    printf("[");
    if (queue != NULL) {
        print_elem(queue);
        queue_t *p = queue->next;
        while (p != queue) {
            printf(" ");
            print_elem(p);
            p = p->next;
        }
    }
    printf("]");
    printf("\n");
}

//------------------------------------------------------------------------------
// Insere um elemento no final da fila.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// - o elemento deve existir
// - o elemento nao deve estar em outra fila
// Retorno: 0 se sucesso, <0 se ocorreu algum erro

int queue_append (queue_t **queue, queue_t *elem) {
    if (queue != NULL) {  
        if (elem != NULL && elem->next == NULL && elem->prev == NULL) {
            if (!(*queue)) {
                *queue = elem;
                (*queue)->next = *queue;
                (*queue)->prev = *queue;
                return 0;
            }

            queue_t *p = (*queue)->prev;
            p->next = elem;
            elem->prev = p;
            elem->next = *queue;
            (*queue)->prev = elem;
            return 0;
        }
    }
    return -1;
}   

//------------------------------------------------------------------------------
// Remove o elemento indicado da fila, sem o destruir.
// Condicoes a verificar, gerando msgs de erro:
// - a fila deve existir
// GRR20190395 Lucas Soni Teixeira

// - a fila nao deve estar vazia
// - o elemento deve existir
// - o elemento deve pertencer a fila indicada
// Retorno: 0 se sucesso, <0 se ocorreu algum erro

int queue_remove (queue_t **queue, queue_t *elem) {
    if ((*queue) != NULL) {
        if (elem != NULL && elem->next != NULL && elem->prev != NULL) {
            if (queue_size(*queue) != 0) {
                if ((*queue) != elem) {
                    queue_t *p = (*queue)->next;
                    while (p != elem && p != (*queue)) {
                        p = p->next;
                    }   
                    if (p == (*queue)) {
                        return -1;
                    }
                    if (queue_size(*queue) > 1) {
                        p->prev->next = p->next;
                        p->next->prev = p->prev;
                    }
                    else {
                        *queue = NULL;
                    }
                        p->prev = NULL;
                        p->next = NULL;
                        return 0;
                    
                }   
                else {
                    if (queue_size(*queue) > 1) {
                        queue_t *aux = (*queue)->next;
                        (*queue)->prev->next = (*queue)->next;
                        (*queue)->next->prev = (*queue)->prev;
                        (*queue)->next = NULL;
                        (*queue)->prev = NULL;
                        (*queue) = aux;
                        return 0;
                    }
                    else {
                        (*queue)->prev = NULL;
                        (*queue)->next = NULL;
                        *queue = NULL;
                        return 0; 
                    }
                }
            }       
        }
    }
    return -1;
}
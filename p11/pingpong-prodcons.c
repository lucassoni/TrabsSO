// PingPongOS - PingPong Operating System
// Prof. Carlos A. Maziero, DINF UFPR
// Versão 1.4 -- Janeiro de 2022

// Teste de semáforos (pesado)

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "ppos.h"

int buffer[5];
int buffer_items = 0;
task_t p1, p2, p3, c1, c2;
semaphore_t s_buffer, s_item, s_vaga;

void produtor(void *arg)
{
   while (true)
   {
      task_sleep(1000);
      int item = random() % 100;

      sem_down(&s_vaga);

      sem_down(&s_buffer);

      if (buffer_items < 5) {
        buffer_items++;
        buffer[buffer_items - 1] = item;
      }
      
      sem_up(&s_buffer);

      sem_up(&s_item);
      printf("%s produziu %d\nbuffer com %d itens\n\n", (char *) arg, item, buffer_items);
   }
}

void consumidor(void *arg)
{
   while (true)
   {
      int item = 9999;
      sem_down(&s_item);

      sem_down(&s_buffer);
      if (buffer_items < 6 && buffer_items > 0) {
          item = buffer[0];
          for (int i = 1; i < buffer_items; i++) {
            buffer[i - 1] = buffer[i];
          }
          buffer_items--;
      }
      sem_up(&s_buffer);

      sem_up(&s_vaga);

      printf("%s consumiu %d\nbuffer com %d itens\n\n", (char *) arg, item, buffer_items);
      task_sleep(1000);
   }
}

int main (int argc, char *argv[])
{

  printf ("main: inicio\n") ;

  ppos_init () ;

  sem_create(&s_vaga, 5);
  sem_create(&s_item, 0);
  sem_create(&s_buffer, 1);

  
  // cria as tarefas
  task_create(&p1, produtor, "p1");
  task_create(&p2, produtor, "p2");
  task_create(&p3, produtor, "p3");
  task_create(&c1, consumidor, "c1");
  task_create(&c2, consumidor, "c2");

  task_join(&p1);
  task_join(&c1);

  task_join(&p2);
  task_join(&c2);

  task_join(&p3);

  sem_destroy(&s_buffer);
  sem_destroy(&s_vaga);
  sem_destroy(&s_item);
}
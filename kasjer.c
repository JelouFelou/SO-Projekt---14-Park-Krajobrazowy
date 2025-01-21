#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include "header.h"
#include <stdlib.h>

extern sem_t semafor_kasjer[K];
extern sem_t semafor_turysta[K];
extern int bilety[K];
extern int wolna_kasa[K];
extern int id_turysty[K];

void *kasjer(void *arg) {
	int id = *(int *)arg;
    free(arg);
	
	while(1){
		sem_wait(&semafor_turysta[id]);
		printf("$$$$$Kasjer %d: Obsługuje turystę %d...\n", id + 1, id_turysty[id]);
		sleep(2);
		printf("$$$$$Kasjer %d: Wydaje bilet na Trasę %d turyście %d\n", id + 1, bilety[id], id_turysty[id]);
		sleep(1);
		sem_post(&semafor_kasjer[id]);
	}
	return NULL;
}
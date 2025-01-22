#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include "header.h"

extern sem_t semafor_kasjer[K];
extern sem_t semafor_turysta[K];
extern int bilety_kasjer[K];
extern int wolna_kasa[K];
extern int id_turysty_kasa[K];
extern volatile int aktywny_park;

void *kasjer(void *arg) {
	int id_kasjer = *(int *)arg;
    free(arg);
	
	while(aktywny_park){
		sem_wait(&semafor_turysta[id_kasjer]);
		if(aktywny_park){
			printf("$$$$$ [Kasjer %d] Obsługuje turystę %d...\n", id_kasjer+1, id_turysty_kasa[id_kasjer]);
			sleep(2);
			printf("$$$$$ [Kasjer %d] Wydaje bilet na Trasę %d turyście %d\n", id_kasjer+1, bilety_kasjer[id_kasjer], id_turysty_kasa[id_kasjer]);
			sleep(1);
		}
		sem_post(&semafor_kasjer[id_kasjer]);
	}
	return NULL;
}
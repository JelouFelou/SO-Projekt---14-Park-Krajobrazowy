#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include "header.h"

extern sem_t semafor_przewodnik[P];
extern sem_t semafor_trasa[P];
extern sem_t semafor_w_trasie[P];

extern volatile int aktywny_park;
extern int wolny_przewodnik[P];
extern int przewodnik_stan[P];
extern int bilety_przewodnik[P];

void *przewodnik(void *arg) {
	int id_przewodnik = *(int *)arg;
	free(arg);
	
	while(aktywny_park) {
		sem_wait(&semafor_przewodnik[id_przewodnik]);
		if(aktywny_park){
			printf(YEL"&&& [Przewodnik %d] Organizuje grupy...\n"RESET, id_przewodnik);
			switch(bilety_przewodnik[id_przewodnik]){
				case 1:
					sem_wait(&semafor_trasa[id_przewodnik]);
					printf(YEL"1 &&& [Przewodnik %d] Oprowadza turystów po trasie 1\n"RESET, id_przewodnik);
					sleep(12);
					printf(YEL"1 &&& [Przewodnik %d] Kończy oprowadzanie turystów po trasie 1\n"RESET, id_przewodnik);
					sleep(1);
					sem_post(&semafor_w_trasie[id_przewodnik]);
					break;
				case 2:
					sem_wait(&semafor_trasa[id_przewodnik]);
					printf(YEL"2 &&& [Przewodnik %d] Oprowadza turystów po trasie 2\n"RESET, id_przewodnik);
					sleep(12);
					printf(YEL"2 &&& [Przewodnik %d] Kończy oprowadzanie turystów po trasie 2\n"RESET, id_przewodnik);
					sleep(1);
					sem_post(&semafor_w_trasie[id_przewodnik]);
					break;
			}
			sleep(2);
		}
		sem_post(&semafor_przewodnik[id_przewodnik]);
	}
	return NULL;
}
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "header.h"

// Deklaracja zmiennych globalnych
extern int wolna_kasa[K];
extern int bilety[K];
extern int id_turysty_kasa[K];
extern sem_t wejscie_do_parku;
extern sem_t semafor_kasjer[K];
extern sem_t semafor_turysta[K];
extern pthread_mutex_t mutex_kasy; 


void *turysta(void *arg) {
	int id = *(int *)arg;
    free(arg);
	
	int czas_pobytu = (rand() % 100) + 1;
	int kupiono_bilet = 0;
	int typ_trasy = (rand() % 2) + 1; // losowanie trasy
	int vip = ((rand() % 100) + 1) == 47;

	
	printf(">>> [Turysta %d] Wchodzi do parku...\n", id);
	
	if(vip){
		printf("*** [Turysta %d] jest VIP'em! Może wejść bez kolejki.***\n", id);
	}else{
		printf(">> [Turysta %d] Zmierza do kasy...\n", id);
		sleep((rand() % 15) + 1); // Temp zmierzanie w stronę kasy
		
		while(!kupiono_bilet){
			// Przeszukiwanie kas w poszukiwaniu wolnej
			for(int i=0;i<K;i++){
				pthread_mutex_lock(&mutex_kasy);
				if(wolna_kasa[i]){
					wolna_kasa[i] = 0; // Kasa o nr. i jest zajęta
					id_turysty_kasa[i] = id; // Przekazanie ID turysty
					pthread_mutex_unlock(&mutex_kasy);
					
					printf("> [Turysta %d] Podchodzi do kasy nr. %d...\n",id, i + 1);
					bilety[i] = typ_trasy;
					printf("? [Turysta %d] Zastanawia się nad wyborem...\n",id);
					
					sem_post(&semafor_turysta[i]);
					sem_wait(&semafor_kasjer[i]);
					
					printf("! [Turysta %d] Odebrał bilet na Trasę %d z kasy nr. %d\n", id, bilety[i], i + 1);
					kupiono_bilet = 1;
					
					pthread_mutex_lock(&mutex_kasy);
					wolna_kasa[i] = 1;
					id_turysty_kasa[i] = -1;
					pthread_mutex_unlock(&mutex_kasy);
					break;
				}
				pthread_mutex_unlock(&mutex_kasy);
			}
			usleep(1000);
		}	
	}
	
	sleep(czas_pobytu);
	printf("----- [Turysta %d] Kończy wizytę w parku, który zwiedzał przez %d s----- \n", id, czas_pobytu);
	sem_post(&wejscie_do_parku);
	pthread_exit(NULL);
}
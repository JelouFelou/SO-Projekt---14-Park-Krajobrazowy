#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <semaphore.h>
#include "header.h"

// Deklaracja zmiennych globalnych
extern int wolna_kasa[K];		// Stan kas
extern sem_t przydzial_kasy; 	// Semafor obsługi kas

void *turysta(void *arg) {
	int id = (int)(long)arg;
	int czas_pobytu = (rand() % 5) + 1;
	int kupiono_bilet = 0;
	
	printf("Turysta %d: Wchodzi do parku...\n", id);
	sleep(2);
	
	while(!kupiono_bilet){
		sem_wait(&przydzial_kasy);
		
		for(int i=0;i<K;i++){
			if(wolna_kasa[i]){
				wolna_kasa[i]=0;
				printf("Turysta %d: Podchodzi do wolnej kasy %d...\n",id, i + 1);
				sleep(2);
				printf("Turysta %d: Kupił bilet w kasie %d...\n",id, i + 1);
				kupiono_bilet = 1;
				wolna_kasa[i] = 1;
				break;
			}
		}
		
		sem_post(&przydzial_kasy);
		
		if(!kupiono_bilet){
			printf("Turysta %d: Nie widzi wolnej kasy zatem czeka...\n", id);
			sleep(2);
		}
	}		
	printf("Turysta %d: Zwiedzał park przez %d s\n",id, czas_pobytu);
	sleep(czas_pobytu);
	printf("Turysta %d: Kończy wizytę w parku \n", id);
	pthread_exit(NULL);
}
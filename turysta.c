#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "header.h"

// Deklaracja zmiennych globalnych
extern int wolna_kasa[K];		// Stan kas
extern sem_t wejscie_do_parku;
//extern sem_t kasa_zajeta;
extern pthread_mutex_t mutex_kasy;

void *turysta(void *arg) {
	int id = *(int *)arg;
    free(arg);
	
	int czas_pobytu = (rand() % 200) + 1;
	int kupiono_bilet = 0;
	int typ_turysty = (rand() % 100) + 1;
	int typ_trasy = (rand() % 2) + 1;
	int vip = (typ_turysty == 47);

	printf(">>Turysta %d: Wchodzi do parku...\n", id);
	
	if(vip){
		printf("***Turysta %d jest VIP'em! Może wejść bez kolejki.***\n", id);
	}else{
		printf("Turysta %d: Czeka w kolejce do kasy...\n", id);
		sleep((rand() % 15) + 1); //temp czekanie w kolejce
		
		while(!kupiono_bilet){
			// Przeszukiwanie kas w poszukiwaniu wolnej
			for(int i=0;i<K;i++){
				pthread_mutex_lock(&mutex_kasy);
				if(wolna_kasa[i]){
					wolna_kasa[i]=0;
					pthread_mutex_unlock(&mutex_kasy);
					
					printf("Turysta %d: Podchodzi do wolnej kasy nr. %d...\n",id, i + 1);
					//sem_wait(&kasa_zajeta);
					
					sleep(2);
					printf("Turysta %d: Zastanawia się nad wyborem...\n",id, i + 1);
					
					switch (typ_trasy){
						case 1:
							printf("$$1$$Turysta %d: Kupił bilet Trasy 1...\n",id);
							break;
						case 2:
							printf("$$2$$Turysta %d: Kupił bilet Trasy 2...\n",id);
							break;
						default:
                            printf("Turysta %d: Nie dokonał zakupu. Opuszcza park.\n", id);
                            sem_post(&wejscie_do_parku);
                            pthread_exit(NULL);
					}
					kupiono_bilet = 1;
					
					pthread_mutex_lock(&mutex_kasy);
					wolna_kasa[i] = 1;
					pthread_mutex_unlock(&mutex_kasy);
					
					//sem_post(&kasa_zajeta);
					break;
				}
				pthread_mutex_unlock(&mutex_kasy);
			}
		}	
	}
	sleep(czas_pobytu);
	printf("-----Turysta %d: Kończy wizytę w parku, który zwiedzał przez %d s-----\n", id, czas_pobytu);
	sem_post(&wejscie_do_parku);
	pthread_exit(NULL);
}
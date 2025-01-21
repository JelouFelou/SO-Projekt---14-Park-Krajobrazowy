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
extern pthread_mutex_t mutex_kasy; // Mutex do synchronizacji dostępu do kas

// Nowe testowe
extern sem_t semafor_kasjer[K];
extern sem_t semafor_turysta[K];
extern int bilety[K];
extern int id_turysty[K];

void *turysta(void *arg) {
	int id = *(int *)arg;
    free(arg);
	
	int czas_pobytu = (rand() % 100) + 1;
	int kupiono_bilet = 0;
	int typ_trasy = (rand() % 2) + 1; // losowanie trasy
	int vip = ((rand() % 100) + 1) == 47;

	
	printf(">>Turysta %d: Wchodzi do parku...\n", id);
	
	if(vip){
		printf("***Turysta %d jest VIP'em! Może wejść bez kolejki.***\n", id);
	}else{
		printf("Turysta %d: Zmierza do kasy...\n", id);
		sleep((rand() % 15) + 1); //temp zmierzanie w stronę kasy
		
		while(!kupiono_bilet){
			// Przeszukiwanie kas w poszukiwaniu wolnej
			for(int i=0;i<K;i++){
				pthread_mutex_lock(&mutex_kasy);
				if(wolna_kasa[i]){
					wolna_kasa[i]=0;
					id_turysty[i] = id;
					pthread_mutex_unlock(&mutex_kasy);
					
					printf("Turysta %d: Podchodzi do kasy nr. %d...\n",id, i + 1);
					// od tego w dół do następnego komentarza nowa wersja kodu
					bilety[i] = typ_trasy; // do konkretnego turysty globalnie wiadomo jaki został przypisany bilet
					
					sem_post(&semafor_turysta[i]);
					sem_wait(&semafor_kasjer[i]);
					printf("Turysta %d: Odebrał bilet na Trasę %d z kasy nr. %d.\n", id, bilety[i], i + 1);
					/*
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
					*/
					kupiono_bilet = 1;
					
					pthread_mutex_lock(&mutex_kasy);
					wolna_kasa[i] = 1;
					id_turysty[i] = -1;
					pthread_mutex_unlock(&mutex_kasy);
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
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <semaphore.h>
#include "header.h"

// Deklaracja zmiennych globalnych
extern int wolna_kasa[K];		// Stan kas
extern sem_t wejscie_do_parku;
//pthread_mutex_t przydzial_kasy = PTHREAD_MUTEX_INITIALIZER;

void *turysta(void *arg) {
	int id = (int)(long)arg;
	int czas_pobytu = (rand() % 200) + 1;
	int kupiono_bilet = 0;
	int typ_turysty = (rand() % 100) + 1;
	int typ_trasy = (rand() % 2) + 1;
	int vip = 0;

	printf(">>Turysta %d: Wchodzi do parku...\n", id);
	
	if(typ_turysty == 47){
		printf("***Turysta %d jest VIP'em!***\n", id);
		vip++;
	}
		
	if(!vip){ // 
		sleep((rand() % 15) + 1);
		while(!kupiono_bilet){
			for(int i=0;i<K;i++){
				if(wolna_kasa[i]){
					wolna_kasa[i]=0;
					printf("Turysta %d: Podchodzi do wolnej kasy nr. %d...\n",id, i + 1);
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
							printf("!!!!!Turysta %d: Nie kupił biletu i zdecydował wyjść z parku...\n",id);
							sem_post(&wejscie_do_parku);
							pthread_exit(NULL);
							break;
					}
					kupiono_bilet = 1;
					wolna_kasa[i] = 1;
					break;
				}
			}
		}	
	}else{
		printf("***Turysta %d: Jako że jest VIP'em posiada legitymację PTTK***\n", id);
		
	}
	
	sleep(czas_pobytu);
	printf("-----Turysta %d: Kończy wizytę w parku, który zwiedzał przez %d s-----\n", id, czas_pobytu);
	sem_post(&wejscie_do_parku);
	pthread_exit(NULL);
}
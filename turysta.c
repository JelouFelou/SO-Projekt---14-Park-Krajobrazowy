#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include "header.h"

// Deklaracja zmiennych globalnych
extern int wolna_kasa[K];
extern int bilety_kasjer[K];
extern int id_turysty_kasa[K];
extern sem_t wejscie_do_parku;
extern sem_t semafor_kasjer[K];
extern sem_t semafor_turysta[K];
extern pthread_mutex_t mutex_kasy; 

//nowy
extern sem_t semafor_przewodnik[P];
extern sem_t semafor_w_trasie[P];
extern sem_t semafor_trasa[P];
extern pthread_mutex_t mutex_przewodnika;
extern pthread_mutex_t mutex_trasa_1;
extern pthread_mutex_t mutex_trasa_2;
extern int wolny_przewodnik[P];
extern int przewodnik_stan[P];
extern int bilety_przewodnik[P];
extern int trasa_1[M];
extern int trasa_2[M];
extern int licznik_trasa_1;
extern int licznik_trasa_2;

void *turysta(void *arg) {
	int id_turysta = *(int *)arg;
    free(arg);
	
	int czas_temp = (rand() % 100) + 1;
	int kupiono_bilet = 0;
	int typ_trasy = (rand() % 2) + 1; // losowanie trasy
	int vip = ((rand() % 100) + 1) == 47;
	
	//nowy

	printf(">>> Do parku wchodzi [Turysta %d]\n", id_turysta);
	
	if(vip){
		printf("*** [Turysta %d] jest VIP'em! Może wejść bez kolejki.***\n", id_turysta);
		sleep(1);
		printf("*** [Turysta %d] jest VIP'em i nie wie co robić.***\n", id_turysta);
	}else{
		printf(">> [Turysta %d] Zmierza do kasy...\n", id_turysta);
		sleep((rand() % 15) + 1); // Temp zmierzanie w stronę kasy
		
		while(!kupiono_bilet){ // kasjer.c
			// Przeszukiwanie kas w poszukiwaniu wolnej
			for(int i=0;i<K;i++){
				pthread_mutex_lock(&mutex_kasy);
				if(wolna_kasa[i]){
					wolna_kasa[i] = 0; // Kasa o nr. i jest zajęta
					id_turysty_kasa[i] = id_turysta; // Przekazanie ID turysty
					pthread_mutex_unlock(&mutex_kasy);
					
					printf("> [Turysta %d] Podchodzi do kasy nr. %d...\n",id_turysta, i + 1);
					bilety_kasjer[i] = typ_trasy;
					printf("? [Turysta %d] Zastanawia się nad wyborem...\n",id_turysta);
					
					sem_post(&semafor_turysta[i]);
					sem_wait(&semafor_kasjer[i]);
					
					printf("! [Turysta %d] Odebrał bilet na Trasę %d z kasy nr. %d\n", id_turysta, bilety_kasjer[i], i + 1);
					kupiono_bilet = 1;
					
					pthread_mutex_lock(&mutex_kasy);
					wolna_kasa[i] = 1;
					id_turysty_kasa[i] = -1;
					pthread_mutex_unlock(&mutex_kasy);
					break;
				}
				pthread_mutex_unlock(&mutex_kasy);
				sleep(1);
				printf("> [Turysta %d] Zmierza na miejsce zbiórki...\n",id_turysta);
				sleep(2);
			}
			usleep(1000);
		}	
		
		for(int i=0;i<P;i++){ // przewodnik.c
			pthread_mutex_lock(&mutex_przewodnika);
				if(wolny_przewodnik[i]){
					if(!przewodnik_stan[i]){ // sprawdzamy czy przewodnik ma przypisaną trasę
						bilety_przewodnik[i] = typ_trasy; // przypisuje trasę do przewodnika
						sem_post(&semafor_przewodnik[i]); // budzi przewodnika
						przewodnik_stan[i]=1; // przewodnik jest aktywny
						printf(RED "INFORMACJA: PRZEWODNIK[&d] UAKTYWNIONY\n" RESET);
					}else{
						printf(GRN "INFORMACJA: PRZEWODNIK[&d] JEST JUŻ AKTYWNY\n" RESET);
					}
					pthread_mutex_unlock(&mutex_przewodnika);
					
					switch(typ_trasy){ //zmierzanie do odpowiedniej trasy
						case 1:
							pthread_mutex_lock(&mutex_trasa_1);
							trasa_1[licznik_trasa_1++] = id_turysta;
							
							if (licznik_trasa_1 == M) {
								wolny_przewodnik[i]=0;
								sem_post(&semafor_trasa[i]); // Grupa trasy 1 gotowa
								sem_wait(&semafor_w_trasie[i]); // Turysta w trasie
							}
							pthread_mutex_unlock(&mutex_trasa_1);
							break;
						case 2:
							pthread_mutex_lock(&mutex_trasa_2);
							trasa_2[licznik_trasa_2++] = id_turysta;
							
							if (licznik_trasa_2 == M) {
								wolny_przewodnik[i]=0;
								sem_post(&semafor_trasa[i]); // Grupa trasy 2 gotowa
								sem_wait(&semafor_w_trasie[i]); // Turysta w trasie
							}
							pthread_mutex_unlock(&mutex_trasa_2);
							break;
					}
					break;
				}
			}
		}

	sleep(czas_temp);
	printf("----- [Turysta %d] Kończy wizytę w parku -----\n", id_turysta);
	
	sem_post(&wejscie_do_parku);
	pthread_exit(NULL);

}
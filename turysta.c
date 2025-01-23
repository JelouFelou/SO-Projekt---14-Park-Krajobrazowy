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
extern int grupa_aktywowana[P];
extern int przypisz_trase[P];
extern int grupa[P];
extern int oprowadza[P];
extern sem_t semafor_grupa[P];
extern sem_t semafor_przewodnik[P];
extern pthread_mutex_t mutex_grupa; 

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
		
		/*
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
		*/
		pthread_mutex_lock(&mutex_grupa);
		for(int i=0;i<P;i++){ // przewodnik.c
			if(!grupa_aktywowana[i]){ 
			//jeżeli grupa nie była aktywowana
				grupa_aktywowana[i] = 1; //aktywujemy
				przypisz_trase[i] = typ_trasy; //przypisujemy trasę grupy
				grupa[i]++; //dodajemy osobę
				
				printf(RED"[Grupa: %d][Turysta %d] Dołączył do grupy trasy %d. Licznik: %d/%d\n"RESET,i+1, id_turysta, typ_trasy, grupa[i], M);
				pthread_mutex_unlock(&mutex_grupa); //zwalniamy mutex umożliwiając resztę przypisanie
				sem_wait(&semafor_grupa[i]); //czekamy na sygnał przewodnika
				break;
			} 
			else if((przypisz_trase[i]==typ_trasy) && grupa[i] < M  && !oprowadza[i]){
			//jeżeli grupa BYŁA aktywowana, ilość osób jest mniejsza niż M i nie oprowadza
				grupa[i]++; //dodajemy osobę
				printf(RED"[Grupa: %d][Turysta %d] Dołączył do grupy trasy %d. Licznik: %d/%d\n"RESET,i+1, id_turysta, typ_trasy, grupa[i], M);
				if(grupa[i] == M){ //sprawdzamy ilość osób
					oprowadza[i]=1; //jak tak to oprowadza włączamy uniemożliwiając wejście do grupy
					pthread_mutex_unlock(&mutex_grupa); //zwalniamy mutex umożliwiając resztę przypisanie do innych grup
					sem_post(&semafor_przewodnik[i]); //budzimy przewodnika
					sem_wait(&semafor_grupa[i]); //czekamy na sygnał przewodnika
				}else{
					pthread_mutex_unlock(&mutex_grupa); //jak if nie zgadza się to zwalniamy mutex
					sem_wait(&semafor_grupa[i]); //czekamy na sygnał przewodnika
				}
				break;
			}
		}
	}
	//sleep(czas_temp);
	printf("----- [Turysta %d] Kończy wizytę w parku -----\n", id_turysta);
	
	sem_post(&wejscie_do_parku);
	pthread_exit(NULL);

}
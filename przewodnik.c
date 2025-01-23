#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdlib.h>
#include "header.h"

extern int grupa_aktywowana[P];
extern int przypisz_trase[P];
extern int grupa[P];
extern int oprowadza[P];
extern sem_t semafor_grupa[P];
extern sem_t semafor_przewodnik[P];
extern pthread_mutex_t mutex_grupa; 

extern volatile int aktywny_park;

void *przewodnik(void *arg) {
	int id_przewodnik = *(int *)arg;
	free(arg);
	
	while (aktywny_park) {
        // Czekamy, aż grupa się zapełni
        sem_wait(&semafor_przewodnik[id_przewodnik]);
        if (!aktywny_park) break; //jeśli zamknięto park, przewodnik zamyka się

        int trasa = przypisz_trase[id_przewodnik];
		
        printf(YEL"&&& [Przewodnik %d] Oprowadza grupę na trasie %d...\n"RESET, id_przewodnik+1, trasa);
        sleep(12); // Symulacja oprowadzania
        printf(YEL"&&& [Przewodnik %d] Kończy oprowadzanie grupy na trasie %d.\n"RESET, id_przewodnik+1, trasa);
		grupa[id_przewodnik]=0;
		przypisz_trase[id_przewodnik] = 0;
		
		for(int i=0; i < M;i++){
			sem_post(&semafor_grupa[id_przewodnik]);
		}
		oprowadza[id_przewodnik] = 0;
		grupa_aktywowana[id_przewodnik] = 0;	
    }
	return NULL;
}
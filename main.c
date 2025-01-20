#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include "header.h"

sem_t wejscie_do_parku; // Semafor ograniczający liczbę turystów w parku
int wolna_kasa[K];

int main() {
	pthread_t /*kasjer_thread, przewodnik_thread,*/ turysta_thread[N];
	srand(time(NULL));
	
	sem_init(&wejscie_do_parku,0,LIMIT);
	
	for(int j=0; j<K; j++){
		wolna_kasa[j]=1;
	}
	
	printf("Testowa symulacja wchodzenia turystów do parku\n\n");
	
	// tworzenie turystów
	for (int i = 1; i < (N+1); i++) {
		sem_wait(&wejscie_do_parku);
		pthread_create(&turysta_thread[i], NULL, turysta, i);
		usleep((rand() % 10000 + 500) * 1000);
	}
	
	// dołączanie wątków turystów
	for (int i = 0; i < N; i++) {
		pthread_join(turysta_thread[i], NULL);
	}
	
	sem_destroy(&wejscie_do_parku);
	printf("\n\nPoprawne zakończenie symulacji\n");
	return 0;
}
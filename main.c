#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "header.h"

pthread_mutex_t mutex_kasy = PTHREAD_MUTEX_INITIALIZER;
sem_t wejscie_do_parku; // Semafor ograniczający liczbę turystów w parku
int wolna_kasa[K];
//sem_t kasa_zajeta;

pthread_t /*kasjer_thread, przewodnik_thread,*/ turysta_thread[N];

int main() {
	srand(time(NULL));
	
	//inicjalizacja semafora, kas i innych zmiennych
	//sem_init(&kasa_zajeta, 0, K);
	sem_init(&wejscie_do_parku,0,LIMIT);
	
	for(int j=0; j<K; j++){
		wolna_kasa[j]=1;
	}
	
	printf("Testowa symulacja wchodzenia turystów do parku\n\n");
	
	// tworzenie wątku kasjera
	//pthread_create(&kasjer_thread, NULL, kasjer, NULL);
	
	// tworzenie wątku turystów
	for (int i = 0; i < N; i++) {
		int *id = malloc(sizeof(int));
        if (!id) {
            perror("Błąd alokacji pamięci");
            exit(EXIT_FAILURE);
        }
        *id = i + 1;
		sem_wait(&wejscie_do_parku);
		if (pthread_create(&turysta_thread[i], NULL, turysta, id) != 0) {
            perror("Błąd tworzenia wątku turysta");
            exit(EXIT_FAILURE);
        }
		usleep((rand() % 10000 + 500) * 1000);
	}
	
	// dołączanie wątków
	for (int i = 0; i < N; i++) {
		pthread_join(turysta_thread[i], NULL);
	}
	//pthread_join(kasjer_thread, NULL);
	
	sem_destroy(&wejscie_do_parku);
	printf("\n\nPoprawne zakończenie symulacji\n");
	return 0;
}
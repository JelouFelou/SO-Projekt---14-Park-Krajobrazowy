#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "header.h"

pthread_mutex_t mutex_kasy = PTHREAD_MUTEX_INITIALIZER;
sem_t wejscie_do_parku; // Semafor ograniczający liczbę turystów w parku
int wolna_kasa[K];

// Nowe testowe
sem_t semafor_kasjer[K]; // semafor (turysta -> kasjer)
sem_t semafor_turysta[K]; // semafor (kasjer -> turysta)
int bilety[K]; // tablia przekazująca informacje o zakupie jakiej trasy (dla kasy)
int id_turysty[K]; // Tablica przechowująca ID turysty (dla kasy)

pthread_t kasjer_thread[K], /*przewodnik_thread[P],*/ turysta_thread[N];

int main() {
	srand(time(NULL));
	
	// inicjalizacja semafora, kas i innych zmiennych
	sem_init(&wejscie_do_parku,0,LIMIT); // semafor ograniczający ilość ludzi w parku
	for (int i = 0; i < K; i++) {
		sem_init(&semafor_turysta[i], 0, 0);
		sem_init(&semafor_kasjer[i], 0, 0); 
	}
	
	for(int i=0; i<K; i++){ // inicjalizacja zmiennej wolnych kas
		wolna_kasa[i]=1; // 1 - kasa jest wolna
	}
	for (int i = 0; i < K; i++) { // inicjalizacja zmiennej typu trasy
		bilety[i] = 0; // 0 - brak biletu
	}
	for (int i = 0; i < K; i++) { // inicjalizacja zmiennej ID turysty
		id_turysty[i] = -1; // -1 - "wyzerowane" ID turysty
	}
	
	
	printf("-------Symulacja parku krajobrazowego-------\n\n");
	
	// tworzenie wątku kasjera
	for (int i = 0; i < K; i++) {
		int *id_kasjer = malloc(sizeof(int));
		if (!id_kasjer) {
            perror("Błąd alokacji pamięci");
            exit(EXIT_FAILURE);
        }
		*id_kasjer = i;
		if (pthread_create(&kasjer_thread[i], NULL, kasjer, id_kasjer) != 0) {
			perror("Błąd tworzenia wątku turysta");
            exit(EXIT_FAILURE);
        }
	}
	
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
	
	// niszczenie semaforów
	sem_destroy(&wejscie_do_parku);
	for (int i = 0; i < K; i++) {
		sem_destroy(&semafor_kasjer[K]);
	}
	
	// zakończenie symulacji
	printf("\n\nPoprawne zakończenie symulacji\n");
	return 0;
}
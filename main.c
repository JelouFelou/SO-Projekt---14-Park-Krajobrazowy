#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include "header.h"

pthread_mutex_t mutex_kasy = PTHREAD_MUTEX_INITIALIZER; // Mutex synchronizacji dostępu kas
sem_t wejscie_do_parku; // Semafor ograniczający liczbę turystów w parku
sem_t semafor_kasjer[K]; // Semafor (turysta -> kasjer)
sem_t semafor_turysta[K]; // Semafor (kasjer -> turysta)
int wolna_kasa[K]; // Tablica przechowująca informację o dostępności kasy
int bilety[K]; // Tablia przekazująca informacje o zakupie typu trasy (dla kasy)
int id_turysty_kasa[K]; // Tablica przechowująca ID turysty (dla kasy)

pthread_t kasjer_thread[K], /*przewodnik_thread[P],*/ turysta_thread[N];
volatile int aktywny_park = 1;

int main() {
	srand(time(NULL));
	
	// inicjalizacje
	if (sem_init(&wejscie_do_parku, 0, LIMIT) != 0) { // Semafor ograniczający liczbę turystów
		perror("Błąd inicjalizacji semafora");
		exit(EXIT_FAILURE);
	}
	for (int i = 0; i < K; i++) {
        if (sem_init(&semafor_turysta[i], 0, 0) != 0 || sem_init(&semafor_kasjer[i], 0, 0) != 0) {
            perror("Błąd inicjalizacji semaforów");
            exit(EXIT_FAILURE);
        }
    }
	for(int i=0; i<K; i++){ // Inicjalizacja zmiennej wolnych kas
		wolna_kasa[i]=1; // 1 - Kasa jest wolna
	}
	for (int i = 0; i < K; i++) { // Inicjalizacja zmiennej typu trasy
		bilety[i] = 0; // 0 - Brak biletu
	}
	for (int i = 0; i < K; i++) { // Inicjalizacja zmiennej ID turysty
		id_turysty_kasa[i] = -1; // -1 - "wyzerowane" ID turysty
	}
	
	
	printf("-------Symulacja parku krajobrazowego-------\n\n");
	
	// Tworzenie wątku kasjera
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
	
	// Tworzenie wątku turystów
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
		// Odczekanie zanim przyjdzie kolejny turysta
		usleep((rand() % 7500 + 500) * 1000);
	}
	
	// Dołączanie wątków
	for (int i = 0; i < N; i++) {
		pthread_join(turysta_thread[i], NULL);
	}
	// Zakończenie pętli kasjerów
	aktywny_park = 0;
    for (int i = 0; i < K; i++) {
        sem_post(&semafor_turysta[i]);
    }
	for (int i = 0; i < K; i++) {
		pthread_join(kasjer_thread[i], NULL);
	}
	
	// Niszczenie semaforów
	sem_destroy(&wejscie_do_parku);
	for (int i = 0; i < K; i++) {
		sem_destroy(&semafor_kasjer[i]);
		sem_destroy(&semafor_turysta[i]);
	}
	
	// Zakończenie symulacji
	printf("\nPoprawne zakończenie symulacji\n");
	return 0;
}
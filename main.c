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
int bilety_kasjer[K]; // Tablia przekazująca informacje o zakupie typu trasy (dla kasy)
int id_turysty_kasa[K]; // Tablica przechowująca ID turysty (dla kasy)


pthread_t kasjer_thread[K], przewodnik_thread[P], turysta_thread[N];
volatile int aktywny_park = 1;


// nowy kod
int grupa_aktywowana[P];
int przypisz_trase[P];
int grupa[P];
int oprowadza[P];
sem_t semafor_grupa[P];
sem_t semafor_przewodnik[P];
pthread_mutex_t mutex_grupa = PTHREAD_MUTEX_INITIALIZER; 


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
		wolna_kasa[i] = 1; // 1 - Kasa jest wolna
		bilety_kasjer[i] = 0; // 0 - Brak biletu
		id_turysty_kasa[i] = -1; // -1 - "wyzerowane" ID turysty
	}
	
	
	// nowe dla przewodnika
	
	for (int i = 0; i < P; i++) {
        if (sem_init(&semafor_grupa[i], 0, 0) != 0 || sem_init(&semafor_przewodnik[i], 0, 0) != 0) {
            perror("Błąd inicjalizacji semaforów");
            exit(EXIT_FAILURE);
        }
		grupa_aktywowana[i] = 0;
		przypisz_trase[i] = 0;
		grupa[i] = 0;
		oprowadza[i] = 0;
	}
	
	// koniec nowego dla przewodnika
	
	
	printf(GRN "-------Symulacja parku krajobrazowego-------\n\n" RESET);
	
	// Tworzenie wątku kasjera
	for (int i = 0; i < K; i++) {
		int *id_kasjer = malloc(sizeof(int));
		if (!id_kasjer) {
            perror("Błąd alokacji pamięci");
            exit(EXIT_FAILURE);
        }
		*id_kasjer = i;
		if (pthread_create(&kasjer_thread[i], NULL, kasjer, id_kasjer) != 0) {
			perror("Błąd tworzenia wątku kasjer");
            exit(EXIT_FAILURE);
        }
	}
	
	// Tworzenie wątku przewodnika
	for (int i = 0; i < P; i++) {
		int *id_przewodnik = malloc(sizeof(int));
		if (!id_przewodnik) {
            perror("Błąd alokacji pamięci");
            exit(EXIT_FAILURE);
        }
		*id_przewodnik = i;
		if (pthread_create(&przewodnik_thread[i], NULL, przewodnik, id_przewodnik) != 0) {
			perror("Błąd tworzenia wątku przewodnik");
            exit(EXIT_FAILURE);
        }
	}
	
	// Tworzenie wątku turystów
	for (int i = 0; i < N; i++) {
		int *id_turysta = malloc(sizeof(int));
        if (!id_turysta) {
            perror("Błąd alokacji pamięci");
            exit(EXIT_FAILURE);
        }
        *id_turysta = i + 1;
		sem_wait(&wejscie_do_parku);
		if (pthread_create(&turysta_thread[i], NULL, turysta, id_turysta) != 0) {
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
	
	// Zamknięcie parku
	aktywny_park = 0;
	
	// Zamykanie kasjerów
    for (int i = 0; i < K; i++) {
        sem_post(&semafor_turysta[i]);
		pthread_join(kasjer_thread[i], NULL);
	}
	
	// Zamykanie przewodników
	for (int i = 0; i < P; i++) {
		sem_post(&semafor_przewodnik[i]);
        pthread_join(przewodnik_thread[i], NULL);
    }
	
	// Niszczenie semaforów
	sem_destroy(&wejscie_do_parku);
	for (int i = 0; i < K; i++) {
        sem_destroy(&semafor_kasjer[i]);
        sem_destroy(&semafor_turysta[i]);
    }
	
	// nowe dla przewodnika
	for (int i = 0; i < P; i++) {
        sem_destroy(&semafor_grupa[i]);
        sem_destroy(&semafor_przewodnik[i]);
    }
	//koniec nowego
	
	
	// Zakończenie symulacji
	printf(GRN "\nPoprawne zakończenie symulacji\n" RESET);
	return 0;
}
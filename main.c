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
int trasa_1[M];
int trasa_2[M];
int licznik_trasa_1;
int licznik_trasa_2;
int bilety_przewodnik[P];
int przewodnik_stan[P];
int wolny_przewodnik[P];
sem_t semafor_przewodnik[P];
sem_t semafor_trasa[P];
sem_t semafor_w_trasie[P];
pthread_mutex_t mutex_trasa_1 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_trasa_2 = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_przewodnika = PTHREAD_MUTEX_INITIALIZER;

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
	
	for (int i = 0; i < P; i++) {
        if (sem_init(&semafor_przewodnik[i], 0, 0) != 0 || sem_init(&semafor_trasa[i], 0, 0) != 0 || sem_init(&semafor_w_trasie[i], 0, 0) != 0) {
            perror("Błąd inicjalizacji semaforów");
            exit(EXIT_FAILURE);
        }
		wolny_przewodnik[i] = 1; // 1 - Przewodnik jest wolny
		przewodnik_stan[i] = 0; // 0 - Przewodnik nie wyczekuje na pełną grupę
		bilety_przewodnik[i] = 0; // 0 - brak informacji o trasie
	}
	
	for (int i = 0; i < M; i++) {
		trasa_1[i] = 0;
		trasa_2[i] = 0;
	}
	
	printf(GRN "-------Symulacja parku krajobrazowego-------\n\n" RESET);
	
	// Tworzenie wątku kasjera
	for (int i = 0; i < K; i++) {
		int *id_kasjer = malloc(sizeof(int));
		if (!id_kasjer) {
            perror("Błąd alokacji pamięci");
            exit(EXIT_FAILURE);
        }
		*id_kasjer = i + 1;
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
		*id_przewodnik = i + 1;
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
	// Zakończenie pętli kasjerów
	aktywny_park = 0;
    for (int i = 0; i < K; i++) {
        sem_post(&semafor_turysta[i]);
		pthread_join(kasjer_thread[i], NULL);
	}
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
    for (int i = 0; i < P; i++) {
        sem_destroy(&semafor_przewodnik[i]);
        sem_destroy(&semafor_trasa[i]);
        sem_destroy(&semafor_w_trasie[i]);
    }
	
	// Zakończenie symulacji
	printf(GRN "\nPoprawne zakończenie symulacji\n" RESET);
	return 0;
}
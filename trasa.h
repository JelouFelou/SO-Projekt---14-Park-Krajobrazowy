#ifndef TRASA_H
#define TRASA_H

#include <sys/ipc.h>
#include <sys/shm.h>
#include "header.h"

#define X1 2   // Maksymalna liczba osób na moście (X1<M)
#define X2 3   // Maksymalna liczba osób na wieży (X2<2M)
#define X3 3   // Maksymalna liczba osób na promie (X3<1.5*M)

// -------Most Wiszący-------
void TrasaA(int IDkolejki, int typ_trasy, int semid_most, int semid_turysta_most, int semid_przewodnik_most, int id_przewodnik, int grupa[], int wiek_turysty[], int liczba_w_grupie) {
    srand(time(NULL));
	// Tworzymy segment pamięci współdzielonej
    key_t key = ftok("header.h", 1);  // Tworzymy unikalny klucz
    int shm_id = shmget(key, sizeof(SharedData), IPC_CREAT | 0666);
    if (shm_id == -1) {
        perror("shmget");
        exit(1);
    }

    SharedData *shm_ptr = (SharedData *)shmat(shm_id, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    // Inicjalizowanie danych w pamięci współdzielonej
    if (shm_ptr->liczba_osob_na_moscie == 0) {
		shm_ptr->liczba_osob_na_moscie = 0;  // Zapobieganie zerowaniu liczby w wypadku dołączenia nowego przewodnika
		shm_ptr->most_kierunek = 0;
	}
	
	printf(GRN "\n-------Most Wiszący-------\n\n" RESET);
    printf("[Przewodnik %d]: Sprawdzam most wiszący...\n", id_przewodnik);
	
	if(shm_ptr->most_kierunek == 0){ // Jeżeli nie ma przypisanej trasy to przypisujemy aktualną
		shm_ptr->most_kierunek=typ_trasy;
		printf("[Przewodnik %d]: Droga wolna! Most jest już dla nas dostępny\n", id_przewodnik);
	}else if(shm_ptr->most_kierunek != typ_trasy){ // Jeżeli trasa która aktualnie tutaj jest to czekamy
		semafor_operacja(semid_most,-1);//ZMIEŃ NA SEMAFOR MIĘDZY PROCESAMI
		shm_ptr->most_kierunek=typ_trasy;
		printf("[Przewodnik %d]: Droga wolna! Most jest już dla nas dostępny\n", id_przewodnik);
	}

	while(1){ // Pierwsze przechodzi przewodnik
		if(shm_ptr->liczba_osob_na_moscie <= X1){
			printf("[Przewodnik %d]: Wchodzę na most jako pierwszy z naszej grupy\n", id_przewodnik);
			shm_ptr->liczba_osob_na_moscie++;
			sleep(rand() % 2 + 1);
			printf("[Przewodnik %d]: Przeszedłem przez most\n", id_przewodnik);
			shm_ptr->liczba_osob_na_moscie--;
			break;
		}
	}
	
	while(1){ // Następnie przechodzą turyści
		if(shm_ptr->liczba_osob_na_moscie <= X1){
			// Wchodzenie turystów na most
			for (int i = 0; i < liczba_w_grupie; i++) {
				shm_ptr->liczba_osob_na_moscie++;
				semafor_operacja(semid_turysta_most, 1);
			}
		}
	}
	
	printf("[Przewodnik %d]: Czekam aż wszyscy przejdą przez most\n", id_przewodnik);
	semafor_operacja(semid_przewodnik_most, -liczba_w_grupie); // Wyczekuje na gotowość wszystkich turystów
    printf("[Przewodnik %d]: Wszyscy z mojej grupy przeszli przez most.\n", id_przewodnik);
	if(shm_ptr->liczba_osob_na_moscie==0){
		shm_ptr->most_kierunek == 0;
		semafor_operacja(semid_most,1);//ZMIEŃ NA SEMAFOR MIĘDZY PROCESAMI
	}
	printf("[Przewodnik %d]: Możemy w takim wypadku iść dalej.\n", id_przewodnik);
}


// -------Wieża Widokowa-------
void TrasaB(int IDkolejki, int semid_wieza, int id_przewodnik, int grupa[], int liczba_w_grupie) {
    printf(GRN "\n-------Wieża Widokowa-------\n\n" RESET);
	printf("[Przewodnik %d]: Wejdźcie na wieże, ja będę czekać na dole\n", id_przewodnik);


	// Wchodzenie turystów na wieżę
    for (int i = 0; i < liczba_w_grupie; i++) {
        semafor_operacja(semid_wieza, -1); // Zajęcie miejsca na wieży
        printf("[Turysta %d]: Wchodzę na wieżę...\n", grupa[i]);
        sleep(rand() % 2 + 1);
    }
    
    sleep(rand() % 5 + 3); // Czas spędzony na wieży

    // Schodzenie turystów z wieży
    for (int i = 0; i < liczba_w_grupie; i++) {
        printf("[Turysta %d]: Schodzę z wieży...\n", grupa[i]);
        sleep(rand() % 2 + 1);
        semafor_operacja(semid_wieza, 1); // Zwolnienie miejsca na wieży
    }

    printf("[Przewodnik %d]: Wszyscy zeszli z wieży.\n", id_przewodnik);
}


// -------Płynięcie promem-------
void TrasaC(int IDkolejki, int semid_prom, int id_przewodnik, int grupa[], int liczba_w_grupie) {
    printf(GRN "\n-------Płynięcie promem-------\n\n" RESET);
	printf("[Przewodnik %d]: Grupa czeka na prom.\n", id_przewodnik);


    printf("[Przewodnik %d]: Prom odpływa...\n", id_przewodnik);
    sleep(rand() % 5 + 3); // Czas przepłynięcia

    printf("[Przewodnik %d]: Prom dopłynął, grupa wysiada.\n", id_przewodnik);
}

#endif
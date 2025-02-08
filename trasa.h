#ifndef TRASA_H
#define TRASA_H

#include "header.h"
#include <time.h>

void LiczbaTurysciTrasy(int typ_trasy, SharedData *shm_ptr);
void PrzeplywPromem(int typ_trasy, int id_przewodnik, int *prom_przewodnik, SharedData *shm_ptr, int semid_przeplyniecie);
void handler_wieza_sygnal(int);

// -------Most Wiszący-------
void TrasaA(int IDkolejki, int typ_trasy, int semid_most, int semid_turysta_most, int semid_przewodnik_most, int id_przewodnik, int grupa[], int liczba_w_grupie) {
	// Inicjalizacja pamięci współdzielonej
	int shm_id;
    SharedData *shm_ptr = shm_init(&shm_id);
	
	printf(GRN "\n-------Most Wiszący-------\n\n" RESET);
    printf("[Przewodnik %d]: Sprawdzam most wiszący...\n", id_przewodnik);
	
	if(shm_ptr->most_kierunek == 0){ // Jeżeli nie ma przypisanej trasy to przypisujemy aktualną
		shm_ptr->most_kierunek=typ_trasy;
		printf("[Przewodnik %d]: Droga wolna! Most jest już dla nas dostępny\n", id_przewodnik);
	}else if(shm_ptr->most_kierunek != typ_trasy){ // Jeżeli aktualnie aktywowana trasa nie jest tą samą co nasza, to czekamy
		shm_ptr->czekajacy_przewodnicy_most++;
		semafor_operacja(semid_most,-1);
		if(shm_ptr->most_kierunek!=typ_trasy){
			shm_ptr->most_kierunek=typ_trasy;
		}
		printf("[Przewodnik %d]: Droga jest już wolna! Most jest już dla nas dostępny\n", id_przewodnik);
	}else if(shm_ptr->most_kierunek == typ_trasy){
		printf("[Przewodnik %d]: Inna grupa porusza się tą samą trasą co my, możemy wejść na ten most\n", id_przewodnik);
	}
	
	shm_ptr->przewodnicy_most++;
	while(1){ // Pierwsze przechodzi przewodnik
		if(shm_ptr->liczba_osob_na_moscie <= X1){
			shm_ptr->liczba_osob_na_moscie++;
			printf("[Przewodnik %d]: Wchodzę na most jako pierwszy z naszej grupy\n", id_przewodnik);
			sleep(rand() % 10 + 1);
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
			break;
		}
	}
	
	printf("[Przewodnik %d]: Czekam aż wszyscy przejdą przez most\n", id_przewodnik);
	semafor_operacja(semid_przewodnik_most, -liczba_w_grupie); // Wyczekuje na gotowość wszystkich turystów
    printf("[Przewodnik %d]: Wszyscy z mojej grupy przeszli przez most.\n", id_przewodnik);
	shm_ptr->przewodnicy_most--;
	
	if(shm_ptr->liczba_osob_na_moscie==0 && shm_ptr->przewodnicy_most==0){
		shm_ptr->most_kierunek = 0;
		if (shm_ptr->czekajacy_przewodnicy_most > 0){
			semafor_operacja(semid_most,shm_ptr->czekajacy_przewodnicy_most);
			shm_ptr->czekajacy_przewodnicy_most=0;
		}
	}
	printf("[Przewodnik %d]: Możemy w takim wypadku iść dalej.\n", id_przewodnik);
	
	// Zwolnienie segmentu pamięci współdzielonej
	shmdt(shm_ptr);
}


// -------Wieża Widokowa-------
void TrasaB(int IDkolejki, int semid_wieza, int semid_turysta_wieza, int semid_przewodnik_wieza, int semid_wieza_limit, int id_przewodnik, int grupa[], int wiek_turysty[], int liczba_w_grupie) {
	// Inicjalizacja pamięci współdzielonej
	int shm_id;
    SharedData *shm_ptr = shm_init(&shm_id);
	
	int i = 0;
	signal(SIGUSR2, handler_wieza_sygnal);
	
	printf(GRN "\n-------Wieża Widokowa-------\n\n" RESET);
	printf("[Przewodnik %d]: Wejdźcie na wieże widokową, ja będę czekać na dole\n", id_przewodnik);
	
	// Wchodzenie turystów na wieżę jedną klatką schodową i schodzenie drugą
	while(i < liczba_w_grupie){
		if(shm_ptr->liczba_osob_na_wiezy <= X2){
			if (wiek_turysty[i] <= 5) {
				// Turysta nie wchodzi:
				printf("[Przewodnik %d]: Turysta %d oraz jego opiekun nie może wejść na wieżę\n", id_przewodnik, grupa[i]);
				i++;
				continue;
			} else {
				// Dopuszczenie turysty do wejścia na wieżę
				shm_ptr->liczba_osob_na_wiezy++;
				semafor_operacja(semid_turysta_wieza, 1);
				printf("[Przewodnik %d]: Z mojej grupy na wieże weszło obecnie %d osób\n",id_przewodnik, i);
				i++;
				continue;
			}
		}else{
			printf("[Przewodnik %d]: Wieża pełna (obecnie: %d osób). Czekam na zwolnienie miejsca...\n",id_przewodnik, shm_ptr->liczba_osob_na_wiezy);
			sleep(5);
			continue;
		}
	}

	printf("[Przewodnik %d]: Czekam aż wszyscy zejdą z wieży widokowej\n", id_przewodnik);
	semafor_operacja(semid_przewodnik_wieza, -liczba_w_grupie); // Wyczekuje na gotowość wszystkich turystów
    printf("[Przewodnik %d]: Wszyscy zeszli z wieży, możemy w takim wypadku iść dalej.\n", id_przewodnik);
	
	// Zwolnienie segmentu pamięci współdzielonej
	shmdt(shm_ptr);
}


// -------Płynięcie promem-------
void TrasaC(int IDkolejki, int typ_trasy, int semid_prom, int semid_turysta_prom, int semid_przeplyniecie, int id_przewodnik, int grupa[], int liczba_w_grupie) {
	// Inicjalizacja pamięci współdzielonej
	int shm_id;
    SharedData *shm_ptr = shm_init(&shm_id);
	
	int prom_przewodnik=0; // 0 - jeszcze nie wszedł, 1 - wszedł, 2 - przepłynął
	int pozostali = liczba_w_grupie;
	
	// Dodaje liczbę osób czekających na konkretnej stronie
	if(typ_trasy==1){
		shm_ptr->turysci_trasa_1 = shm_ptr->turysci_trasa_1 + liczba_w_grupie;
	}else if(typ_trasy==2){
		shm_ptr->turysci_trasa_2 = shm_ptr->turysci_trasa_2 + liczba_w_grupie;
	}
	
	printf(BLU "\n-------Płynięcie promem-------\n\n" RESET);
	printf("[Przewodnik %d]: Sprawdzam prom...\n", id_przewodnik);
	
	if(shm_ptr->prom_kierunek==0){
		shm_ptr->prom_kierunek=typ_trasy;
	}
	
	// Pętla wykonuje się puki wszyscy nie przepłynęli
	while(pozostali > 0) {
		// Sprawdza czy prom jest na jego stronie, jeśli nie czeka poprzez semafor oraz dopisana jest liczba czekających przewodników
		if(shm_ptr->prom_kierunek != typ_trasy && prom_przewodnik==0) {
            printf(YEL"Prom znajduje się po drugiej stronie, trzeba czekać.\n"RESET);
			shm_ptr->czekajacy_przewodnicy_most++;
			semafor_operacja(semid_prom,-1);
        }	
		
		// --- Zapełnianie promu
		// Pierwsze wchodzi przewodnik
		if(prom_przewodnik == 0 && shm_ptr->prom_zajete < X3){
			shm_ptr->prom_zajete++;
			prom_przewodnik=1;
			printf("[Przewodnik %d]: Wchodzę na prom jako pierwszy.\n", id_przewodnik);
		}
		
		// Wchodzą turyści
		while(pozostali > 0 && shm_ptr->prom_zajete < X3){
			shm_ptr->prom_zajete++;
			LiczbaTurysciTrasy(typ_trasy, shm_ptr);
			pozostali--;
			printf(YEL "[DEBUG] Prom zajęte: %d/%d. Pozostało turystów w grupie: %d.\n" RESET,shm_ptr->prom_zajete, X3, pozostali);
			if (typ_trasy == 1) {
				printf(YEL "[DEBUG] Pozostało czekających turystów z trasy %d: %d\n" RESET,typ_trasy, shm_ptr->turysci_trasa_1);
			} else if (typ_trasy == 2) {
				printf(YEL "[DEBUG] Pozostało czekających turystów z trasy %d: %d\n" RESET,typ_trasy, shm_ptr->turysci_trasa_2);
			}
			semafor_operacja(semid_turysta_prom, 1);
		}
		
		// --- Odpływanie Promu
		// Jeżeli prom zapełniony to przepływa na drugą stronę
		if (shm_ptr->prom_zajete == X3 || (shm_ptr->prom_zajete > 0 && ((typ_trasy == 1 && shm_ptr->turysci_trasa_1 == 0) || (typ_trasy == 2 && shm_ptr->turysci_trasa_2 == 0)))) {
			PrzeplywPromem(typ_trasy, id_przewodnik, &prom_przewodnik, shm_ptr, semid_przeplyniecie);
		}
		
		// Jeżeli nie ma turystów po żadnej ze stron
		if (shm_ptr->turysci_trasa_1 == 0 && shm_ptr->turysci_trasa_2 == 0) {
			printf(BLU "[Prom]: Nie ma już turystów, prom pozostaje gotowy na następną grupę.\n" RESET);
		} else {
			shm_ptr->prom_kierunek = (shm_ptr->prom_kierunek == 1) ? 2 : 1;
			printf(BLU"[Prom]: Odpłynął na stronę %d\n"RESET,typ_trasy);
			sleep(TPROM);
			printf(BLU"[Prom]: Dopłynął na stronę %d\n"RESET,typ_trasy);
		}
		
		// Jeżeli wszyscy turyści z danej grupy przepłynęli to kończymy pętle
		if(pozostali==0){
			break;
		}
    }

    printf("[Przewodnik %d]: Wszyscy z mojej grupy przepłynęli promem. Kontynuujemy wycieczkę.\n", id_przewodnik);

	// Zwolnienie segmentu pamięci współdzielonej
    shmdt(shm_ptr);
}


void LiczbaTurysciTrasy(int typ_trasy, SharedData *shm_ptr) {
    if (typ_trasy == 1) {
        shm_ptr->turysci_trasa_1--;
    } else if (typ_trasy == 2) {
        shm_ptr->turysci_trasa_2--;
    }
}

void PrzeplywPromem(int typ_trasy, int id_przewodnik, int *prom_przewodnik, SharedData *shm_ptr, int semid_przeplyniecie) {
    // Zamiana strony
    shm_ptr->prom_kierunek = (typ_trasy == 1) ? 2 : 1;
	
	printf(BLU"[Prom]: Odpłynął na stronę %d\n"RESET,typ_trasy);
    sleep(TPROM);
    printf(BLU"[Prom]: Dopłynął na stronę %d\n"RESET,typ_trasy);

    // Wysiadają pasażerowie
    for (int i = 0; i < X3; i++) {
        if (*prom_przewodnik == 1) {  // Przewodnik wysiada pierwszy
            printf("[Przewodnik %d]: Jestem po drugiej stronie i czekam na moją grupę\n", id_przewodnik);
            *prom_przewodnik = 2;
            shm_ptr->prom_zajete--;
        } else {  // Turyści wysiadają
            if (shm_ptr->prom_zajete > 0) {
                shm_ptr->prom_zajete--;
                semafor_operacja(semid_przeplyniecie, 1);
            }
        }
    }
}

void handler_wieza_sygnal(int sig) {
    int shm_id;
    SharedData *shm_ptr = shm_init(&shm_id);
	printf(YEL"[Przewodnik]: Proszę wszystkich o zejście z wieży.\n"RESET);
	shm_ptr->wieza_sygnal = 1;
	sleep(5);
	shm_ptr->wieza_sygnal = 0;
}
#endif
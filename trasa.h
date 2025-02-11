#ifndef TRASA_H
#define TRASA_H

#include "header.h"
#include <time.h>


int prom_liczba = 0; // liczba turystów, którzy już weszli na prom
int start_id = 0; // Liczba od której sprawdzamy czy dany turysta wszedł na prom

void LiczbaTurysciTrasy(int typ_trasy, SharedData *shm_ptr);
void PrzeplywPromem(int IDkolejki, int typ_trasy, int id_przewodnik, int grupa[], int semid_prom, int prom_przewodnik, int start_id, int prom_liczba, SharedData *shm_ptr);
void handler_wieza_sygnal(int);

/*
// -------Most Wiszący-------
void TrasaA(int IDkolejki, int typ_trasy, int semid_most, int semid_turysta_most, int semid_przewodnik_most, int id_przewodnik, int grupa[], int liczba_w_grupie) {
	// Inicjalizacja pamięci współdzielonej
	int shm_id;
    SharedData *shm_ptr = shm_init(&shm_id);
	struct komunikat kom;
	int i = 0;
	
	printf(GRN "\n-------Most Wiszący-------\n\n" RESET);
    printf("[%d][Przewodnik %d]: Sprawdzam most wiszący...\n",typ_trasy, id_przewodnik);
	
// --- Sprawdzanie dostępności wieży
	if(shm_ptr->most_kierunek == 0){ // Jeżeli nie ma przypisanej trasy to przypisujemy aktualną
		shm_ptr->most_kierunek = typ_trasy;
		printf("[%d][Przewodnik %d]: Droga wolna! Most jest już dla nas dostępny\n",typ_trasy, id_przewodnik);
	}else if(shm_ptr->most_kierunek != typ_trasy){ // Jeżeli aktualnie aktywowana trasa nie jest tą samą co nasza, to czekamy
		shm_ptr->czekajacy_przewodnicy_most++;
		semafor_operacja(semid_most,-1);
		
		if(shm_ptr->most_kierunek!=typ_trasy){
			shm_ptr->most_kierunek=typ_trasy;
		}
		printf("[%d][Przewodnik %d]: Droga jest już wolna! Most jest już dla nas dostępny\n",typ_trasy, id_przewodnik);
	}else if(shm_ptr->most_kierunek == typ_trasy){
		printf("[%d][Przewodnik %d]: Inna grupa porusza się tą samą trasą co my, możemy wejść na ten most\n",typ_trasy, id_przewodnik);
	}
	
// --- Wchodzenie na most przewodnika
	shm_ptr->przewodnicy_most++;
	while(1){ // Pierwsze przechodzi przewodnik
		if(shm_ptr->liczba_osob_na_moscie <= X1){
			shm_ptr->liczba_osob_na_moscie++;
			printf("[%d][Przewodnik %d]: Wchodzę na most jako pierwszy z naszej grupy\n",typ_trasy, id_przewodnik);
			sleep(rand() % 10 + 1);
			printf("[%d][Przewodnik %d]: Przeszedłem przez most\n",typ_trasy, id_przewodnik);
			shm_ptr->liczba_osob_na_moscie--;
			break;
		}
	}
	
// --- Wchodzenie na most turystów
	while(i < liczba_w_grupie){
		if(shm_ptr->liczba_osob_na_moscie <= X1){
			//1. Dopuszczenie turysty do wejścia na wieżę
			shm_ptr->liczba_osob_na_moscie++;
			kom.mtype = grupa[i];
			sprintf(kom.mtext, "OK");
			if (msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0) == -1) {
				perror("msgsnd failed (Most)");
			} else {
				printf("[%d][Przewodnik %d]: Wysłano sygnał do turysty %d\n", typ_trasy, id_przewodnik, grupa[i]);
			}
			printf("[%d][Przewodnik %d]: Z mojej grupy na most weszło obecnie %d osób\n",typ_trasy,id_przewodnik, i);
			i++;
			continue;
		} else {
			printf("[%d][Przewodnik %d]: Most pełny (obecnie: %d osób). Czekamy na zwolnienie miejsca...\n",typ_trasy,id_przewodnik, shm_ptr->liczba_osob_na_moscie);
			sleep(2);
			continue;
		}
	}
	
	printf("[%d][Przewodnik %d]: Czekam aż wszyscy przejdą przez most\n",typ_trasy, id_przewodnik);
	//2. Wyczekuje na gotowość wszystkich turystów
	for (i = 0; i < liczba_w_grupie; i++) {
        if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, id_przewodnik, 0) == -1) {
            perror("msgrcv failed (Most - DONE)");
        } else {
            // Można opcjonalnie sprawdzić, czy treść komunikatu to "DONE"
            printf("[%d][Przewodnik %d]: Otrzymano potwierdzenie od turysty (%s)\n", typ_trasy, id_przewodnik, kom.mtext);
        }
    }
	
    printf("[%d][Przewodnik %d]: Wszyscy z mojej grupy przeszli przez most.\n",typ_trasy, id_przewodnik);
	shm_ptr->przewodnicy_most--;
	
// --- Jeżeli most jest pusty oraz nie ma przewodników na moście to puszczamy czekających przewodników
	if(shm_ptr->liczba_osob_na_moscie==0 && shm_ptr->przewodnicy_most==0){
		shm_ptr->most_kierunek = 0;
		if (shm_ptr->czekajacy_przewodnicy_most > 0){
			semafor_operacja(semid_most,shm_ptr->czekajacy_przewodnicy_most);
			shm_ptr->czekajacy_przewodnicy_most=0;
		}
	}
	printf("[%d][Przewodnik %d]: Możemy w takim wypadku iść dalej.\n",typ_trasy, id_przewodnik);
	
	// Zwolnienie segmentu pamięci współdzielonej
	shmdt(shm_ptr);
}


// -------Wieża Widokowa-------
void TrasaB(int IDkolejki, int typ_trasy, int semid_wieza, int semid_turysta_wieza, int semid_przewodnik_wieza, int semid_wieza_limit, int id_przewodnik, int grupa[], int wiek_turysty[], int liczba_w_grupie) {
	// Inicjalizacja pamięci współdzielonej
	int shm_id;
    SharedData *shm_ptr = shm_init(&shm_id);
	struct komunikat kom;
	
	int i = 0;
	signal(SIGUSR2, handler_wieza_sygnal);
	
	printf(GRN "\n-------Wieża Widokowa-------\n\n" RESET);
	printf("[%d][Przewodnik %d]: Wejdźcie na wieże widokową, ja będę czekać na dole\n",typ_trasy, id_przewodnik);
	
// --- Wchodzenie turystów na wieżę jedną klatką schodową i schodzenie drugą
	while(i < liczba_w_grupie){
		if(shm_ptr->liczba_osob_na_wiezy <= X2){
			if (wiek_turysty[i] <= 5) {
			// Turysta nie wchodzi:
				printf("[%d][Przewodnik %d]: Turysta %d oraz jego opiekun nie może wejść na wieżę\n",typ_trasy, id_przewodnik, grupa[i]);
				kom.mtype = grupa[i];
				sprintf(kom.mtext, "OK");
				msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
				i++;
				continue;
			} else {
			//1. Dopuszczenie turysty do wejścia na wieżę
				shm_ptr->liczba_osob_na_wiezy++;
				kom.mtype = grupa[i];
				sprintf(kom.mtext, "OK");
				msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
				printf("[%d][Przewodnik %d]: Z mojej grupy na wieże weszło obecnie %d osób\n",typ_trasy,id_przewodnik, i);
				i++;
				continue;
			}
		}else{
			printf("[%d][Przewodnik %d]: Wieża pełna (obecnie: %d osób). Czekam na zwolnienie miejsca...\n",typ_trasy,id_przewodnik, shm_ptr->liczba_osob_na_wiezy);
			sleep(5);
			continue;
		}
	}

	printf("[Przewodnik %d]: Czekam aż wszyscy zejdą z wieży widokowej\n", id_przewodnik);
	//2. Wyczekuje na gotowość wszystkich turystów
	for (int i = 0; i < liczba_w_grupie; i++) {
		if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, id_przewodnik, 0) == -1){
			perror("msgrcv failed");
		}
	}
    printf("[Przewodnik %d]: Wszyscy zeszli z wieży, możemy w takim wypadku iść dalej.\n", id_przewodnik);
	
	// Zwolnienie segmentu pamięci współdzielonej
	shmdt(shm_ptr);
}

*/
// -------Płynięcie promem-------
void TrasaC(int IDkolejki, int typ_trasy, int semid_prom, int semid_turysta_wchodzenie, int semid_czekajaca_grupa, int id_przewodnik, int grupa[], int liczba_w_grupie) {
	// Inicjalizacja pamięci współdzielonej
	int shm_id;
    SharedData *shm_ptr = shm_get(&shm_id);
	struct komunikat kom;
	
	int prom_przewodnik=0; // 0 - jeszcze nie wszedł, 1 - wszedł, 2 - przepłynął
	int info = 0;
	
	// Dodaje liczbę osób czekających na konkretnej stronie
	if(typ_trasy==1){
		shm_ptr->turysci_trasa_1 += liczba_w_grupie;
	}else if(typ_trasy==2){
		shm_ptr->turysci_trasa_2 += liczba_w_grupie;
	}
	
	printf(BLU "\n-------Płynięcie promem-------\n\n" RESET);
	printf("[%d][Przewodnik %d]: Sprawdzam stan promu...\n",typ_trasy, id_przewodnik);
	sleep(1);
	
	// Ustawiamy stronę promu
	if(shm_ptr->prom_kierunek==0){
		shm_ptr->prom_kierunek=typ_trasy;
		printf("[%d][Przewodnik %d]: Prom znajduje się po naszej %d stronie\n",typ_trasy, id_przewodnik, shm_ptr->prom_kierunek);
	}
	prom_liczba = 0;
	start_id = 0;
// --- Pętla wykonuje się puki wszyscy nie przepłynęli

	while(prom_liczba < liczba_w_grupie) {
		if(info==0){
			printf("[%d][Przewodnik %d]: Prom kierunek: %d, oczekiwany: %d\n",typ_trasy, id_przewodnik, shm_ptr->prom_kierunek, typ_trasy);
			info=1;
		}
		
	// Grupa czeka do momentu aż prom znajdzie się na ich stronie
		if(shm_ptr->prom_kierunek != typ_trasy) { // Informacja o czekających grupach
			if(prom_przewodnik==0) printf(YEL"[%d][Przewodnik %d]: Prom znajduje się po drugiej stronie, trzeba czekać.\n"RESET,typ_trasy, id_przewodnik);
			else printf(YEL"[INFO]: Prom znajduje się po drugiej stronie, trzeba czekać.\n"RESET);
			
			shm_ptr->czekajace_grupy_prom++;
			printf(YEL"[INFO]: Czekające grupy: %d\n"RESET,shm_ptr->czekajace_grupy_prom);
			semafor_operacja(semid_prom,-1);
			printf(YEL"[INFO]: Widzimy zmierzający w naszą stronę prom!\n"RESET);
			sleep(1);
			printf("[%d][Przewodnik %d]: Prom kierunek: %d, oczekiwany: %d\n",typ_trasy, id_przewodnik, shm_ptr->prom_kierunek, typ_trasy);
			continue;
        }
	
// --- Zapełnianie promu	
	// Pierwsze wchodzi przewodnik a po nim wchodzą turyści
		while(prom_liczba < liczba_w_grupie && shm_ptr->prom_zajete < X3 && shm_ptr->prom_odplynal==0 && shm_ptr->prom_kierunek == typ_trasy){
		// Blokuje dostęp do wchodzenia wszystkim
			if(shm_ptr->turysta_blokada==1){
				//shm_ptr->turysta_wchodzenie++;
				semafor_operacja(semid_turysta_wchodzenie, -1); // Blokujemy dostęp do dalszej części programu dla 1 grupy
				continue;
			}
			shm_ptr->turysta_blokada=1;
			
		// Przewodnik	
			if(prom_przewodnik == 0 && shm_ptr->prom_zajete < X3 && shm_ptr->prom_odplynal==0 && shm_ptr->prom_kierunek == typ_trasy){
				shm_ptr->prom_zajete++;
				prom_przewodnik=1;
				printf("[%d][Przewodnik %d]: Wchodzę na prom jako pierwszy (miejsc zajętych: %d/%d)\n",typ_trasy, id_przewodnik, shm_ptr->prom_zajete, X3);
			} else if (shm_ptr->prom_odplynal==1){
				printf("[%d][Przewodnik %d]: Prom już odpłynął\n",typ_trasy, id_przewodnik);
				semafor_operacja(semid_turysta_wchodzenie, 1); // Odblokujemy dostęp do pętli dla kolejnej grupy
				break;
			}
			else if (shm_ptr->prom_zajete == X3){
				printf("[%d][Przewodnik %d]: Prom jest już zapełniony\n",typ_trasy, id_przewodnik);
				semafor_operacja(semid_turysta_wchodzenie, 1); // Odblokujemy dostęp do pętli dla kolejnej grupy
				break;
			}
			
		// Turyści		
			if (shm_ptr->prom_zajete < X3 && shm_ptr->prom_odplynal==0 && shm_ptr->prom_kierunek == typ_trasy){
				shm_ptr->prom_zajete++;
				kom.mtype = grupa[prom_liczba] + PROM_START_OFFSET;
				sprintf(kom.mtext, "START");
				if (msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0) == -1) {
					perror("msgsnd failed - START");
				}
				LiczbaTurysciTrasy(typ_trasy, shm_ptr); // Zmniejsza liczbę osób danej strony o 1
				printf("[%d][Przewodnik %d]: Turysta %d wchodzi na prom (miejsc zajętych: %d/%d)\n",typ_trasy, id_przewodnik, grupa[prom_liczba], shm_ptr->prom_zajete, X3);
				prom_liczba++;
				shm_ptr->turysta_blokada=0;
				semafor_operacja(semid_turysta_wchodzenie, 1); // Odblokujemy dostęp do pętli dla kolejnej grupy
			} else if (shm_ptr->prom_odplynal==1){
				printf("[%d][Przewodnik %d]: Prom już odpłynął\n",typ_trasy, id_przewodnik);
				semafor_operacja(semid_turysta_wchodzenie, 1); // Odblokujemy dostęp do pętli dla kolejnej grupy
				break;
			}
			else if (shm_ptr->prom_zajete == X3){
				printf("[%d][Przewodnik %d]: Prom jest już zapełniony\n",typ_trasy, id_przewodnik);
				semafor_operacja(semid_turysta_wchodzenie, 1); // Odblokujemy dostęp do pętli dla kolejnej grupy
				break;
			}
			
			info=0;
			
		// Warunkowe wyjścia z pętli
			if((prom_liczba == liczba_w_grupie) && ((typ_trasy == 1 && shm_ptr->turysci_trasa_1 == 0) || (typ_trasy == 2 && shm_ptr->turysci_trasa_2 == 0))){
				printf("[Prom]: Grupa przewodnika %d jest ostatnią po tej stronie, więcej turystów już nie wejdzie\n",id_przewodnik);
				break;
			}
			else if (prom_liczba == liczba_w_grupie){
				printf("[Prom]: Wszystkie osoby z grupy przewodnika %d weszły na prom\n",id_przewodnik);
				shm_ptr->czekajaca_grupa++;
				semafor_operacja(semid_czekajaca_grupa, -1);
				PrzeplywPromem(IDkolejki, typ_trasy, id_przewodnik, grupa, semid_prom, prom_przewodnik, start_id, prom_liczba, shm_ptr);
				shm_ptr->prom_zajete = 0;
				start_id = prom_liczba; // Przypisujemy prom_liczba do start_id aby następne wykonanie funkcji zaczęło się od turysty który jeszcze nie wysiadł
				if(prom_liczba == liczba_w_grupie){
					break;
				}
			}
		}
// --- Odpływ Promu
		if (shm_ptr->prom_odplynal==0 && shm_ptr->prom_zajete == X3 || (shm_ptr->prom_zajete > 0 && ((typ_trasy == 1 && shm_ptr->turysci_trasa_1 == 0) || (typ_trasy == 2 && shm_ptr->turysci_trasa_2 == 0)))) {
			if(shm_ptr->czekajaca_grupa>0){
				semafor_operacja(semid_czekajaca_grupa, shm_ptr->czekajaca_grupa);
			}
			PrzeplywPromem(IDkolejki, typ_trasy, id_przewodnik, grupa, semid_prom, prom_przewodnik, start_id, prom_liczba, shm_ptr);
			shm_ptr->prom_zajete = 0;
			start_id = prom_liczba; // Przypisujemy prom_liczba do start_id aby następne wykonanie funkcji zaczęło się od turysty który jeszcze nie wysiadł
			info=0;
			if(prom_liczba == liczba_w_grupie){
				break;
			}
		}
    }
// --- Wszyscy przepłynęli na drugą stronę	
	
	//3. Wyczekuje na gotowość wszystkich turystów
	for (int i = 0; i < liczba_w_grupie; i++) {
		if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, id_przewodnik + PROM_READY_OFFSET, 0) == -1) {
            perror("msgrcv failed (Prom - waiting for DONE)");
		} else {
			if (strcmp(kom.mtext, "DONE") == 0) {
				//printf("[%d][Przewodnik %d]: Otrzymano potwierdzenie od turysty %d po przepłynięciu (%s)\n",typ_trasy, id_przewodnik, grupa[i], kom.mtext);
			} else {
				printf("[Przewodnik %d]: Otrzymałem nieoczekiwany komunikat: %s\n", id_przewodnik, kom.mtext);
			}
		}
	}
    printf("[%d][Przewodnik %d]: Wszyscy z mojej grupy przepłynęli promem. Kontynuujemy wycieczkę.\n",typ_trasy, id_przewodnik);

	// Zwolnienie segmentu pamięci współdzielonej
    shmdt(shm_ptr);
}


// --- Funkcje pomocnicze
void PrzeplywPromem(int IDkolejki, int typ_trasy, int id_przewodnik, int grupa[], int semid_prom, int prom_przewodnik, int start_id, int prom_liczba, SharedData *shm_ptr) {
	struct komunikat kom;
	int blokada=0;
	
// --- Zamiana strony / Przepływ na drugą stronę
	
	shm_ptr->prom_odplynal = 1;
	if(shm_ptr->prom_blokada==0){ // zapewnienie, że wielu przewodników nie będzie chciało kilkukrotnie zmieniac kierunek przepływu promu
		printf(BLU"[Prom]: Odpłynął z strony %d\n"RESET,shm_ptr->prom_kierunek);
		shm_ptr->prom_blokada = 1;
		blokada = 1;
		shm_ptr->prom_kierunek = (typ_trasy == 1) ? 2 : 1;
	}
	
    sleep(TPROM);
    if(blokada == 1) printf(BLU"[Prom]: Dopłynął na stronę %d\n"RESET,shm_ptr->prom_kierunek);

// --- Wysiadają pasażerowie
	// Przewodnik wysiada jeżeli był na promie
	if (prom_przewodnik == 1) {  
        printf("[%d][Przewodnik %d]: Jestem po drugiej stronie i czekam na moją grupę\n",typ_trasy, id_przewodnik);
		prom_przewodnik = 2;
        shm_ptr->prom_zajete--; // Przewodnik opuszcza prom
    }

	//2. Wysiadają turyści
    for (int i = start_id; i < prom_liczba; i++) {
		sleep(1);
        shm_ptr->prom_zajete--;
		kom.mtype = grupa[i] + PROM_EXIT_OFFSET;
		sprintf(kom.mtext, "PROM");
		if (msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0) == -1) {
            perror("msgsnd failed (Prom)");
        }else{
			printf("[%d][Przewodnik %d]: Turysta %d wysiada z promu\n",typ_trasy, id_przewodnik, grupa[i]);
		}
    }
	
// --- Sprawdzanie aktualnego stanu Wycieczki Promem przez wszystkie grupy
	// Jeżeli na stronie na której aktualnie jest prom nie ma nikogo, ale są po drugiej stronie, prom wraca 
	if(blokada==1&&((shm_ptr->prom_kierunek==1 && shm_ptr->turysci_trasa_1==0 && shm_ptr->turysci_trasa_2>0) || (shm_ptr->prom_kierunek==2 && shm_ptr->turysci_trasa_2==0 && shm_ptr->turysci_trasa_1>0))){
		printf(BLU"[Prom]: Zmiana kierunku. Po tej stronie nie ma nikogo. Odpłynął z strony %d\n"RESET, shm_ptr->prom_kierunek);
		shm_ptr->prom_kierunek = (typ_trasy == 1) ? 1 : 2;
		sleep(TPROM);
		printf(BLU"[Prom]: Dopłynął na stronę %d\n"RESET,shm_ptr->prom_kierunek);
		
		// Odblokujemy czekające grupy jeżeli takie są
		if(shm_ptr->czekajace_grupy_prom>0){
			printf(BLU"[Prom]: Sygnalizuje czekającym przewodnikom, że mogą już wsiadać\n"RESET);
			semafor_operacja(semid_prom,shm_ptr->czekajace_grupy_prom);
			shm_ptr->czekajace_grupy_prom=0; // Zerujemy licznik czekających grup
		}
		
	// Jeżeli na żadnej ze stron nie ma turystów, prom ustawia kierunek na 0
	}else if(blokada==1&&(shm_ptr->turysci_trasa_1==0 && shm_ptr->turysci_trasa_2==0)){
		printf(BLU"[Prom]: Na żadnej ze stron nie ma już turystów\n"RESET);
		shm_ptr->prom_kierunek=0;
	}
	
	if(blokada==1){
		shm_ptr->prom_blokada = 0;
		shm_ptr->prom_odplynal = 0;
		blokada=0;
	}
}

void LiczbaTurysciTrasy(int typ_trasy, SharedData *shm_ptr) {
    if (typ_trasy == 1) {
        shm_ptr->turysci_trasa_1--;
    } else if (typ_trasy == 2) {
        shm_ptr->turysci_trasa_2--;
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
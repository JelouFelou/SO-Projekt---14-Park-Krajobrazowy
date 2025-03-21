#ifndef TRASA_H
#define TRASA_H

#include "header.h"
#include <time.h>

SharedData *global_shm_ptr = 0;

int prom_liczba = 0; // liczba turystów, którzy już weszli na prom
int start_id = 0; // Liczba od której sprawdzamy czy dany turysta wszedł na prom
int numer_wieza = 0;

void PrzeplywPromem(int IDkolejki, int typ_trasy, int id_przewodnik, int grupa[], int semid_prom, int prom_przewodnik, int start_id, int prom_liczba, SharedData *shm_ptr);
void handler_wieza_sygnal(int);


// -------Most Wiszący-------
void TrasaA(int IDkolejki, int typ_trasy, int semid_most, int semid_most_wchodzenie, int id_przewodnik, int grupa[], int liczba_w_grupie, int wydluzenie, SharedData *shm_ptr) {
	struct komunikat kom;
	int i = 0;
	int most_przewodnik=0;
	
	float czas = rand() % 10 + 1;
	if(wydluzenie==1){
		czas *= 1.5;
	}
	
	//printf(GRN "\n-------Most Wiszący-------\n\n" RESET);
    printf("[%d][Przewodnik %d]: Sprawdzam most wiszący...\n",typ_trasy, id_przewodnik);
	
// --- Sprawdzanie dostępności wieży
	semafor_operacja(semid_most_wchodzenie, -1);
	if(shm_ptr->most_kierunek == 0){ // Most wolny, ustawiamy kierunek
		shm_ptr->most_kierunek = typ_trasy;
		printf("[%d][Przewodnik %d]: Droga wolna! Most jest już dla nas dostępny\n",typ_trasy, id_przewodnik);
		semafor_operacja(semid_most_wchodzenie, 1);
	}else if(shm_ptr->most_kierunek != typ_trasy){ // Most zajęty przez inną trasę, czekamy
		shm_ptr->czekajacy_przewodnicy_most++;
		semafor_operacja(semid_most_wchodzenie, 1);
		semafor_operacja(semid_most,-1);
		shm_ptr->most_kierunek=typ_trasy;
		printf("[%d][Przewodnik %d]: Droga jest już wolna! Most jest już dla nas dostępny\n",typ_trasy, id_przewodnik);
	}else if(shm_ptr->most_kierunek == typ_trasy){
		printf("[%d][Przewodnik %d]: Inna grupa porusza się tą samą trasą co my, możemy wejść na ten most\n",typ_trasy, id_przewodnik);
		semafor_operacja(semid_most_wchodzenie, 1);
	}
	
	while(i < liczba_w_grupie){
	// Przewodnik
		if(most_przewodnik == 0 && shm_ptr->liczba_osob_na_moscie <= X1){
			shm_ptr->przewodnicy_most++;
			shm_ptr->liczba_osob_na_moscie++;
			printf("[%d][Przewodnik %d]: Wchodzę na most jako pierwszy z naszej grupy\n",typ_trasy, id_przewodnik);
			sleep(czas);
			printf("[%d][Przewodnik %d]: Przeszedłem przez most\n",typ_trasy, id_przewodnik);
			shm_ptr->liczba_osob_na_moscie--;
			most_przewodnik=1;
		}else if(most_przewodnik == 0 && shm_ptr->liczba_osob_na_moscie >= (X1-1)) {
			printf("[%d][Przewodnik %d]: Most pełny (obecnie: %d osób). Czekamy na zwolnienie miejsca...\n",typ_trasy,id_przewodnik, shm_ptr->liczba_osob_na_moscie-1);
			sleep(2);
			continue;
		}
	// Turysta	
		if(shm_ptr->liczba_osob_na_moscie <= X1){
			shm_ptr->liczba_osob_na_moscie++;
			
			kom.mtype = grupa[i];
			sprintf(kom.mtext, "OK %f", czas);
			msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
			
			//printf("[%d][Przewodnik %d]: Wysłano sygnał do turysty %d\n", typ_trasy, id_przewodnik, grupa[i]);
			sleep(1);
			printf("[%d][Przewodnik %d]: Z mojej grupy na most weszło obecnie %d osób\n",typ_trasy,id_przewodnik, i);
			i++;
			continue;
		} else {
			printf("[%d][Przewodnik %d]: Most pełny (obecnie: %d osób). Czekamy na zwolnienie miejsca...\n",typ_trasy,id_przewodnik, shm_ptr->liczba_osob_na_moscie);
			sleep(5);
			continue;
		}
	}
		
	
	printf("[%d][Przewodnik %d]: Czekam aż wszyscy przejdą przez most\n",typ_trasy, id_przewodnik);
	//2. Wyczekuje na gotowość wszystkich turystów
	for (i = 0; i < liczba_w_grupie; i++) {
        if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, id_przewodnik, 0) == -1) {
            perror("msgrcv failed (Most - DONE)");
        } else {
            //printf("[%d][Przewodnik %d]: Otrzymano potwierdzenie od turysty (%s)\n", typ_trasy, id_przewodnik, kom.mtext);
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
}


// -------Wieża Widokowa-------
void TrasaB(int IDkolejki, int typ_trasy, int semid_wieza, int id_przewodnik, int grupa[], int wiek_turysty[], int liczba_w_grupie, int wydluzenie, int numer, SharedData *shm_ptr) {
	struct komunikat kom;
	
	int numer_wieza = numer;
	int i = 0;
	float czas = rand() % 10 + 1;
	if(wydluzenie==1){
		czas *= 1.5;
	}
	
	shm_ptr->wieza_sygnal[numer_wieza] = 0;
	signal(SIGCHLD, handler_wieza_sygnal);
	
	//printf(GRN "\n-------Wieża Widokowa-------\n\n" RESET);
	printf("[%d][Przewodnik %d]: Wejdźcie na wieże widokową, ja będę czekać na dole\n",typ_trasy, id_przewodnik);
	
// --- Wchodzenie turystów na wieżę jedną klatką schodową i schodzenie drugą
	while(i < liczba_w_grupie){
		if(shm_ptr->liczba_osob_na_wiezy <= X2){
			if (wiek_turysty[i] <= 5) {
			// Turysta nie wchodzi:
				printf("[%d][Przewodnik %d]: Turysta %d oraz jego opiekun nie może wejść na wieżę\n",typ_trasy, id_przewodnik, grupa[i]);
				kom.mtype = grupa[i];
				sprintf(kom.mtext, "OK %f", czas);
				msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
				i++;
				continue;
			} else {
			//1. Dopuszczenie turysty do wejścia na wieżę
				shm_ptr->liczba_osob_na_wiezy++;
				kom.mtype = grupa[i];
				sprintf(kom.mtext, "OK %f", czas);
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

	printf("[%d][Przewodnik %d]: Czekam aż wszyscy zejdą z wieży widokowej\n",typ_trasy, id_przewodnik);
	//2. Wyczekuje na gotowość wszystkich turystów
	for (int i = 0; i < liczba_w_grupie; i++) {
		if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, id_przewodnik, 0) == -1){
			//perror("msgrcv failed");
		}
	}
    printf("[%d][Przewodnik %d]: Wszyscy zeszli z wieży, możemy w takim wypadku iść dalej.\n",typ_trasy, id_przewodnik);
}


// -------Płynięcie promem-------
void TrasaC(int IDkolejki, int typ_trasy, int semid_prom_check, int semid_prom, int semid_turysta_wchodzenie, int semid_czekajaca_grupa, int id_przewodnik, int grupa[], int liczba_w_grupie, int wiek_turysty[], SharedData *shm_ptr) {
	struct komunikat kom;
	
	int prom_przewodnik=0; // 0 - jeszcze nie wszedł, 1 - wszedł, 2 - przepłynął
	int info = 0;
	int wydluzenie = 0;
	
	
	//printf(BLU "\n-------Płynięcie promem-------\n\n" RESET);
	printf("[%d][Przewodnik %d]: Sprawdzam stan promu...\n",typ_trasy, id_przewodnik);
	sleep(1);
	
	// Ustawiamy stronę promu
	semafor_operacja(semid_prom_check, -1);
	if(shm_ptr->prom_kierunek==0){
		shm_ptr->prom_kierunek=typ_trasy;
		printf("[%d][Przewodnik %d]: Prom znajduje się po naszej stronie: %d\n",typ_trasy, id_przewodnik, typ_trasy);
	}else{
		printf("[%d][Przewodnik %d]: Prom nie znajduje się po naszej stronie: %d\n",typ_trasy, id_przewodnik, typ_trasy);
	}
	
	// Dodaje liczbę osób czekających na konkretnej stronie
	if(typ_trasy==1){
		//printf(YEL"%d ----------------------trasa 1: przed wejściem 1: %d\n"RESET,(int)time(NULL),shm_ptr->turysci_trasa_1);
		//printf(YEL"%d ----------------------trasa 2: przed wejściem 1: %d\n"RESET,(int)time(NULL),shm_ptr->turysci_trasa_2);
		shm_ptr->turysci_trasa_1 += liczba_w_grupie;
		//printf(YEL"%d --------------------------trasa 1: po wejściu 1: %d\n"RESET,(int)time(NULL),shm_ptr->turysci_trasa_1);
		//printf(YEL"%d --------------------------trasa 2: po wejściu 1: %d\n"RESET,(int)time(NULL),shm_ptr->turysci_trasa_2);
	}else if(typ_trasy==2){
		//printf(YEL"%d ----------------------trasa 1: przed wejściem 2: %d\n"RESET,(int)time(NULL),shm_ptr->turysci_trasa_1);
		//printf(YEL"%d ----------------------trasa 2: przed wejściem 2: %d\n"RESET,(int)time(NULL),shm_ptr->turysci_trasa_2);
		shm_ptr->turysci_trasa_2 += liczba_w_grupie;
		//printf(YEL"%d --------------------------trasa 1: po wejściu 2: %d\n"RESET,(int)time(NULL),shm_ptr->turysci_trasa_1);
		//printf(YEL"%d --------------------------trasa 2: po wejściu 2: %d\n"RESET,(int)time(NULL),shm_ptr->turysci_trasa_2);
	}
	semafor_operacja(semid_prom_check, 1);
	
	prom_liczba = 0;
// --- Pętla wykonuje się puki wszyscy nie przepłynęli

	while(prom_liczba < liczba_w_grupie) {
	
// --- Zapełnianie promu	
	// Pierwsze wchodzi przewodnik a po nim wchodzą turyści	
		while(prom_liczba < liczba_w_grupie && shm_ptr->prom_zajete < X3 && shm_ptr->prom_kierunek == typ_trasy && shm_ptr->prom_odplynal==0){
		// Blokuje dostęp do wchodzenia wszystkim przewodnikom
			semafor_operacja(semid_turysta_wchodzenie, -1); // Blokujemy dostęp do dalszej części programu dla reszty			
			
		// W wypadku gdy przejdzie przez semafor i typ trasy jest inny albo prom jest aktualnie niedostępny
			if(shm_ptr->prom_kierunek != typ_trasy || shm_ptr->prom_odplynal==1){
				semafor_operacja(semid_turysta_wchodzenie, 1);
				continue;
			}	
		//1. Odczytuje komunikat OK od promu
			if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, PROM + PROM_START_OFFSET, 0) == -1) {
				perror("msgrcv failed");
			}
			
		//2.1. Przewodnik
			if(prom_przewodnik == 0 && shm_ptr->prom_zajete < X3 && shm_ptr->prom_kierunek == typ_trasy){
				prom_przewodnik=1;
				kom.mtype = PROM + PROM_ENTER_OFFSET;
				sprintf(kom.mtext, "[Przewodnik %d] chce wejść na prom", id_przewodnik);
				msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
				if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, PROM + PROM_WELCOME_OFFSET, 0) == -1) {
					perror("msgrcv failed");
					semafor_operacja(semid_turysta_wchodzenie, 1);
				} 
				else {
					sleep(1);
					printf("[%d][Przewodnik %d]: Wchodzę na prom jako pierwszy (miejsc zajętych: %d/%d)\n",typ_trasy, id_przewodnik, shm_ptr->prom_zajete, X3);
					semafor_operacja(semid_turysta_wchodzenie, 1);
					continue;
				}
			}
			
		//2.2. Turyści		
			if (shm_ptr->prom_zajete < X3 && shm_ptr->prom_kierunek == typ_trasy){
			// Wysyłana jest informacja do promu, że turysta wchodzi na prom
				if(wiek_turysty[prom_liczba]<12){
					wydluzenie=1;
				}
				kom.mtype = PROM + PROM_ENTER_OFFSET;
				sprintf(kom.mtext, "[Turysta %d] chce wejść na prom %d", grupa[prom_liczba], wydluzenie);
				msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
				if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, PROM + PROM_WELCOME_OFFSET, 0) == -1) {
					perror("msgrcv failed");
					semafor_operacja(semid_turysta_wchodzenie, 1);
				} else {
				//Wysyłana jest informacja do turysty by wszedł na prom
					kom.mtype = grupa[prom_liczba] + PROM_START_OFFSET;
					sprintf(kom.mtext, "START");
					msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
				
					sleep(1);
					printf("[%d][Przewodnik %d]: Turysta %d wchodzi na prom (miejsc zajętych: %d/%d) (1:%d 2:%d) (stan danej grupy: %d/%d)\n",typ_trasy, id_przewodnik, grupa[prom_liczba], shm_ptr->prom_zajete, X3, shm_ptr->turysci_trasa_1, shm_ptr->turysci_trasa_2, prom_liczba+1, liczba_w_grupie);
					prom_liczba++;
					semafor_operacja(semid_turysta_wchodzenie, 1); // Odblokujemy dostęp do pętli dla kolejnej grupy
				}
			} 
			
			if (prom_liczba == liczba_w_grupie){
				printf("[INFO]: Wszystkie osoby z grupy przewodnika %d weszły na prom\n",id_przewodnik);
				break;
			}else if(shm_ptr->prom_zajete == X3){
				break;
			}
		}
		//printf("główne while: %d; czy %d < %d? czy prom odpłynął %d? Kierunek a typ trasy: %d/%d\n", id_przewodnik, prom_liczba, liczba_w_grupie, shm_ptr->prom_odplynal, shm_ptr->prom_kierunek, typ_trasy);
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
}

// --- Funkcje pomocnicze

void handler_wieza_sygnal(int sig) {
	int id_przewodnik = getpid();
	printf(YEL"[Przewodnik %d]: Proszę wszystkich o zejście z wieży.\n"RESET, id_przewodnik);
	global_shm_ptr->wieza_sygnal[numer_wieza] = 1;
}
#endif
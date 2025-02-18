#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <string.h>
#include "header.h"

void przedwczesne_wyjscie(int);
void TurystaMost(int IDkolejki, int id_przewodnik, int wiek, int id_turysta);
void TurystaWieza(int IDkolejki, int id_przewodnik, int wiek, int id_turysta);
void TurystaProm(int IDkolejki, int id_przewodnik, int wiek, int id_turysta);

int main() {
	srand(time(NULL));
	struct komunikat kom;
	int id_turysta = getpid();
	int id_przewodnik = 0;
	int id_kasjer = 0;
	int vip = (rand() % 100 == 47);
	int typ_trasy = 0;
	int wiek = 0;
	
	
	while(1){
		if (!czy_istnieje(id_kasjer)) continue;
		else if (!czy_istnieje(id_turysta)) continue;
		else break;
	}
	// Inicjalizacja pamięci współdzielonej
	int shm_id;
    SharedData *shm_ptr = shm_get(&shm_id);
	
	// Jeśli turysta jest VIP-em – ustawiamy losowo trasę oraz wiek
	if(vip){
		typ_trasy = (rand() % 2) + 1;
		wiek = (rand() % 80) + 1;
	}
	// Dla zwykłego turysty dane (typ trasy i wiek) zostaną nadane przez kasjera
	
    key_t key_kolejka;
	int IDkolejki;

	if(shm_ptr->liczba_turystow==N){
		printf("Osiągnięto już limit turystów danego dnia\n");
		exit(1);
	}else{
		shm_ptr->liczba_turystow++;
		shm_ptr->start_max=0;
	}
	
	signal(SIGTERM,przedwczesne_wyjscie);
	
	printf(GRN "-------Symulacja parku krajobrazowego - Turysta %d-------\n\n" RESET,id_turysta);

    // Pobieranie ID kolejki komunikatów
	key_kolejka = ftok(".", 99);
    if ((IDkolejki = msgget(key_kolejka, IPC_CREAT | 0600)) == -1) {
        perror("msgget() błąd");
        exit(1);
    }
	

// Początek symulacji turysty
	printf(">>> Do parku wchodzi [Turysta %d]\n", id_turysta);
	sleep(1);
	
	if(!vip){ // Turysta nie jest vipem
		printf("> [Turysta %d] podchodzi do kasy\n",id_turysta);
	
	// ---- Kasjer ----
	
		//1. Zgłoszenie do kolejki
		kom.mtype = KASJER;
		sprintf(kom.mtext, "[Turysta %d] zgłasza się do kolejki", id_turysta);
		msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);

		//2. Oczekiwanie na wezwanie do kasy
		if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, id_turysta, 0) == -1) {
			perror("msgrcv failed");
		} else {
			printf("[Turysta %d] został wezwany do kasy\n", id_turysta);
			sscanf(kom.mtext, "Zapraszamy do kasy - od %d", &id_kasjer);
		}
	
		//3. Komunikat do kasjera – wysyłamy dalej już na dedykowany kanał
		kom.mtype = id_kasjer;
		sprintf(kom.mtext, "[Turysta %d] chce kupić bilet", id_turysta);
		msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
		printf("[Turysta %d] przekazuje kasjerowi %d, że chce kupić bilet\n", id_turysta, id_kasjer);

		//4. Odbiór biletu
		if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, id_turysta, 0) == -1) {
			perror("msgrcv failed");
		} else {
			printf("[Turysta %d] odbiera %s\n\n", id_turysta, kom.mtext);
			sscanf(kom.mtext, "bilet na trasę %d dla osoby z wiekiem %d. Ma pójść do przewodnika %d", &typ_trasy, &wiek, &id_przewodnik);
		}
		
	// ---- Przewodnik ----
		sleep(2);
		printf("[Turysta %d] czeka na rozpoczęcie oprowadzania...\n", id_turysta);
		
		// Turysta czeka aż przewodnik zacznie wycieczkę
		kom.mtype = id_turysta;
		if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, id_turysta, 0) == -1) {
			perror("msgrcv failed");
		} else {
			printf("[Turysta %d] jest podekscytowany zwiedzaniem parku!\n", id_turysta);
		}
	
		// Wycieczka zależna od typu trasy
		switch(typ_trasy){
			case(1):
				//TurystaMost(IDkolejki, id_przewodnik, wiek, id_turysta);
				TurystaWieza(IDkolejki, id_przewodnik, wiek, id_turysta);
				//TurystaProm(IDkolejki, id_przewodnik, wiek, id_turysta);
				break;
			case(2):
				//TurystaProm(IDkolejki, id_przewodnik, wiek, id_turysta);
				TurystaWieza(IDkolejki, id_przewodnik, wiek, id_turysta);
				//TurystaMost(IDkolejki, id_przewodnik, wiek, id_turysta);
				break;
		}
		
		//X. Koniec wycieczki
		printf(GRN "\n-------Koniec wycieczki-------\n\n" RESET);
		if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, id_turysta, 0) == -1){
			perror("msgrcv failed");
		}
		
	} else { // Turysta jest vipem
		printf(YEL"[Turysta %d] jest vipem\n"RESET, id_turysta);
		sleep(2);
		switch(typ_trasy){
			case(1):
				break;
			case(2):
				break;
		}
	}
	
    printf("[Turysta %d] opuszcza park po zakończeniu trasy\n", id_turysta);
	
	shmdt(shm_ptr);
    return 0;
}


void przedwczesne_wyjscie(int sig_n){
	printf("[Turysta] przedwcześnie opuszcza park\n");
    exit(1);
}

void TurystaMost(int IDkolejki, int id_przewodnik, int wiek, int id_turysta){
	int shm_id;
    SharedData *shm_ptr = shm_get(&shm_id);
	struct komunikat kom;
	
//1. Turysta czeka na sygnał od przewodnika
	if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, id_turysta, 0) == -1) {
		perror("msgrcv failed");
	}
	
// --- Rozpoczęcie mostu wiszącego
	printf(GRN "\n-------Most Wiszący-------\n\n" RESET);
	sleep(TMOST);
	if(wiek<15){
		printf("[Turysta %d]: Wchodzę na most pod opieką osoby dorosłej...\n", id_turysta);
	}else{
		printf("[Turysta %d]: Wchodzę na most...\n", id_turysta);
	}
	sleep(1);
	printf("[Turysta %d]: Podziwia widoki\n", id_turysta);
	sleep(TMOST);
	printf("[Turysta %d]: Dotarłem na koniec mostu...\n", id_turysta);
	shm_ptr->liczba_osob_na_moscie--;
	
//2. Powiadomienie przewodnika, że turysta przeszedł przez most
	kom.mtype = id_przewodnik;
	sprintf(kom.mtext, "DONE");
	if (msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0) == -1) {
        perror("msgsnd failed (Most - DONE)");
    }
	
	shmdt(shm_ptr);
}

void TurystaWieza(int IDkolejki, int id_przewodnik, int wiek, int id_turysta){
	int shm_id;
    SharedData *shm_ptr = shm_get(&shm_id);
	struct komunikat kom;
	
//1. Turysta czeka na sygnał od przewodnika
	if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, id_turysta, 0) == -1) {
        perror("msgrcv failed (Wieża)");
    }
	
// --- Rozpoczęcie wchodzenia na wieżę widokową
	printf(GRN "\n-------Wieża Widokowa-------\n\n" RESET);
	// Turysta wchodzi na wieżę jedną klatką schodową
	if(wiek<=5){
		printf("[Turysta %d]: Czekam pod wieżą aż reszta grupy zejdzie...\n", id_turysta);
	}else if(wiek<15){
		printf("[Turysta %d]: Wchodzę na wieżę pod opieką osoby dorosłej...\n", id_turysta);
	}else{
		printf("[Turysta %d]: Wchodzę na wieżę...\n", id_turysta);
	}
	
	//Turysta na wieży
	if(wiek>5){
		sleep(TWIEZA);
		printf("[Turysta %d]: Wszedłem na wieżę...\n", id_turysta);
		sleep(1);
		printf("[Turysta %d]: Podziwia widoki...\n", id_turysta);
	
	// Turysta schodzi z wieży drugą klatką schodową
		if(shm_ptr->wieza_sygnal == 1){
			printf("[Turysta %d]: Natychmiastowo schodzi z wieży...\n", id_turysta);
			shm_ptr->liczba_osob_na_wiezy--;
		}else{
			printf("[Turysta %d]: Zchodzi z wieży...\n", id_turysta);
			sleep(TWIEZA);
			printf("[Turysta %d]: Zszedł z wieży i czeka na resztę...\n", id_turysta);
			shm_ptr->liczba_osob_na_wiezy--;
		}
	}
	sleep(1);
	
//2. Powiadomienie przewodnika, że turysta zszedł z wieży
    kom.mtype = id_przewodnik;
    sprintf(kom.mtext, "DONE");
    if (msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0) == -1) {
        perror("msgsnd failed (Wieża - DONE)");
    }
	
	shmdt(shm_ptr);
}

void TurystaProm(int IDkolejki, int id_przewodnik, int wiek, int id_turysta){
	int shm_id;
    SharedData *shm_ptr = shm_get(&shm_id);
	struct komunikat kom;
	
//1. Turysta czeka na sygnał od przewodnika
	if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, id_turysta + PROM_START_OFFSET, 0) == -1) {
        perror("msgrcv failed (Prom -START)");
    } else {
        if (strcmp(kom.mtext, "START") == 0) {
            printf("[Turysta %d]: Widzi prom\n", id_turysta);
        } else {
            printf("[Turysta %d]: Otrzymałem nieoczekiwany komunikat: %s\n", id_turysta, kom.mtext);
        }
    }
	
// --- Rozpoczęcie płynięcia promem
	printf(BLU "\n-------Płynięcie promem-------\n\n" RESET);
			
	if(wiek<15){
		printf("[Turysta %d]: Wchodzę na prom pod opieką osoby dorosłej...\n", id_turysta);
	}else{
		printf("[Turysta %d]: Wchodzę na prom...\n", id_turysta);
	}
	sleep(2);
	printf("[Turysta %d]: Czekam na promie...\n", id_turysta);
//2. Turysta wchodzi na prom 
	if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, id_turysta + PROM_EXIT_OFFSET, 0) == -1) {
        perror("msgrcv failed (Prom - PROM)");
    } else {
        if (strcmp(kom.mtext, "PROM") == 0) {
            printf("[Turysta %d]: Przeplynąłem promem na drugą stronę\n", id_turysta);
        } else {
            printf("[Turysta %d]: Otrzymałem nieoczekiwany komunikat: %s\n", id_turysta, kom.mtext);
        }
    }
	sleep(2);
	printf("[Turysta %d]: Czekam na resztę mojej grupy wraz z przewodnikiem\n", id_turysta);
//3. Powiadomienie przewodnika, że turysta jest po drugiej stronie
    kom.mtype = id_przewodnik + PROM_READY_OFFSET;
    sprintf(kom.mtext, "DONE");
    if (msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0) == -1) {
        perror("msgsnd failed (Prom - DONE)");
    }

	shmdt(shm_ptr);
}
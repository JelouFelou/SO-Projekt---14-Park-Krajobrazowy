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
void TurystaMost();
void TurystaWieza();
void TurystaProm();

int main() {
	srand(time(NULL));
	struct komunikat kom;
	int id_turysta = getpid();
	int vip = (rand() % 100 == 47);
	int typ_trasy = 0;
	int wiek = 0;
	
	if(vip){
		int typ_trasy = (rand() % 2) + 1;
		int wiek = (rand() % 80) + 1;
	}
	
    key_t key_kolejka, key_semafor_wyjscie, key_semafor_wycieczka;
    key_t key_turysta_most, key_turysta_wieza, key_turysta_prom;
	key_t key_przewodnik_most, key_przewodnik_wieza, key_przewodnik_prom;
	key_t key_przeplyniecie, key_wieza_limit;
	int IDkolejki, semid_kasa, semid_wyjscie, semid_wycieczka;
	int semid_turysta_most, semid_turysta_wieza, semid_turysta_prom;
	int semid_przewodnik_most, semid_przewodnik_wieza, semid_przewodnik_prom;
	int semid_przeplyniecie, semid_wieza_limit;
	
	// Inicjalizacja pamięci współdzielonej
	int shm_id;
    SharedData *shm_ptr = shm_init(&shm_id);
	
	if(shm_ptr->liczba_turystow==N){
		printf("Osiągnięto już limit turystów danego dnia\n");
		exit(1);
	}else{
		shm_ptr->liczba_turystow++;
	}
	
	signal(SIGTERM,przedwczesne_wyjscie);
	
	printf(GRN "-------Symulacja parku krajobrazowego - Turysta %d-------\n\n" RESET,id_turysta);

    // Pobieranie ID kolejki komunikatów
	key_kolejka = ftok(".", 99);
    if ((IDkolejki = msgget(key_kolejka, IPC_CREAT | 0600)) == -1) {
        perror("msgget() błąd");
        exit(1);
    }
	
	// Koniec wycieczki
	key_semafor_wyjscie = ftok(".", 100);
	if ((semid_wyjscie = semget(key_semafor_wyjscie, 1, IPC_CREAT | 0600)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	
	// Odpowiedzialny za informacje o rozpoczęciu wycieczki
	key_semafor_wycieczka = ftok(".", 101);
	if ((semid_wycieczka = semget(key_semafor_wycieczka, 1, IPC_CREAT | 0600)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	
	// Odpowiedzialne za sterowanie turystą podczas pobytu na moście, wieży i promie
	key_turysta_most = ftok(".", 102);
	if ((semid_turysta_most = semget(key_turysta_most, 1, IPC_CREAT | 0600)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	key_turysta_wieza = ftok(".", 103);
	if ((semid_turysta_wieza = semget(key_turysta_wieza, 1, IPC_CREAT | 0600)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	key_turysta_prom = ftok(".", 104);
	if ((semid_turysta_prom = semget(key_turysta_prom, 1, IPC_CREAT | 0600)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	
	// Odpowiedzialne za sterowanie przewodnikiem podczas pobytu na moście, wieży i promie
	key_przewodnik_most = ftok(".", 105);
	if ((semid_przewodnik_most = semget(key_przewodnik_most, 1, IPC_CREAT | 0600)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	key_przewodnik_wieza = ftok(".", 106);
	if ((semid_przewodnik_wieza = semget(key_przewodnik_wieza, 1, IPC_CREAT | 0600)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	key_przewodnik_prom = ftok(".", 107);
	if ((semid_przewodnik_prom = semget(key_przewodnik_prom, 1, IPC_CREAT | 0600)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	key_przeplyniecie = ftok(".", 108);
	if ((semid_przeplyniecie = semget(key_przeplyniecie, 1, IPC_CREAT | 0600)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	key_wieza_limit = ftok(".", 109);
	if ((semid_wieza_limit = semget(key_wieza_limit, 1, IPC_CREAT | 0600)) == -1) {
        perror("semget() błąd");
        exit(1);
    }

// Początek symulacji turysty
	printf(">>> Do parku wchodzi [Turysta %d]\n", id_turysta);
	sleep(1);
	
	if(!vip){ // Turysta nie jest vipem
		printf("> [Turysta %d] podchodzi do kasy\n\n",id_turysta);
	
	// ---- Kasjer ----
	
		//1. Zgłoszenie do kolejki
		sprintf(kom.mtext, "[Turysta %d] zgłasza się do kolejki", id_turysta);
		kom.mtype = KASJER;
		msgsnd(IDkolejki, (struct msgbuf *)&kom, strlen(kom.mtext) + 1, 0);

		//2. Oczekiwanie na wezwanie do kasy
		if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, id_turysta, 0) == -1) {
			perror("msgrcv failed");
		} else {
			printf("[Turysta %d] został wezwany do kasy\n", id_turysta);
		}
		
		// Wyodrębnienie PID kasjera z komunikatu
		int id_kasjer = 0;
		char *ptr = strstr(kom.mtext, "od ");
		if (ptr != NULL) {
			id_kasjer = atoi(ptr + 3);
		} else {
			fprintf(stderr, "Nie udało się odczytać PID kasjera\n");
			exit(1);
		}
	
		//3. Komunikat do kasjera – wysyłamy dalej już na dedykowany kanał
		kom.mtype = id_kasjer;
		sprintf(kom.mtext, "[Turysta %d] chce kupić bilet",id_turysta);
		printf("[Turysta %d] przekazuje kasjerowi %d, że chce kupić bilet\n", id_turysta, id_kasjer);
		msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);

		//4. Odbiór biletu
		if (msgrcv(IDkolejki, &kom, MAX, id_turysta, 0) == -1)  {
			perror("msgrcv failed");
		} else {
			printf("[Turysta %d] odbiera %s\n\n", id_turysta, kom.mtext);
			sscanf(kom.mtext, "bilet na trasę %d dla osoby z wiekiem %d", &typ_trasy, &wiek);
		}
		
	// ---- Przewodnik ----
	
		// Próba przyjęcia turysty przez przewodnika
		if (msgrcv(IDkolejki, &kom, MAX, id_turysta, 0) == -1) {
			perror("msgrcv final message failed");
			exit(1);
		} else {
			// Turysta został przyjęty
			if (strncmp(kom.mtext, "OK", 2) == 0) { 
				printf("[Turysta %d] otrzymałem potwierdzenie od kasjera: %s\n\n", id_turysta, kom.mtext);
				if (sscanf(kom.mtext, "OK %d", &typ_trasy) == 1) {
				} else {
					printf("[Turysta %d] Otrzymałem nieoczekiwany komunikat: %s\n", id_turysta, kom.mtext);
					exit(1);
				}
			}
			// Turysta jest odrzucony, zmiana trasy
			else if (strncmp(kom.mtext, "REJECT", 6) == 0) { 
				if (sscanf(kom.mtext, "REJECT %d", &typ_trasy) == 1) {
					printf("[Turysta %d] Otrzymałem propozycję zmiany trasy. Nowa trasa: %d.\n", id_turysta, typ_trasy);
					// Następuje powtórzenie pętli – wysłanie nowego żądania z aktualnym typem trasy.
				} else {
					printf("[Turysta %d] Otrzymałem nieoczekiwany komunikat: %s\n", id_turysta, kom.mtext);
					exit(1);
				}
			} else {
				printf("[Turysta %d] otrzymałem nieoczekiwany komunikat: %s\n", id_turysta, kom.mtext);
				exit(1);
			}
		}
	
		sleep(2);
		printf("[Turysta %d] otrzymał bilet i idzie do przewodnika\n", id_turysta);
		printf("[Turysta %d] czeka na rozpoczęcie oprowadzania...\n", id_turysta);
		
		semafor_operacja(semid_wycieczka, -1); // Turysta czeka aż przewodnik zacznie wycieczkę
		printf("[Turysta %d] jest podekscytowany zwiedzaniem parku!\n", id_turysta);
	
		// Wycieczka zależna od typu trasy
		switch(typ_trasy){
			case(1):
				TurystaMost(semid_turysta_most, semid_przewodnik_most, wiek, id_turysta);
				TurystaWieza(semid_turysta_wieza, semid_przewodnik_wieza, wiek, id_turysta);
				TurystaProm(semid_turysta_prom, semid_przewodnik_prom, semid_przeplyniecie, wiek, id_turysta);
				break;
			case(2):
				TurystaProm(semid_turysta_prom, semid_przewodnik_prom, semid_przeplyniecie, wiek, id_turysta);
				TurystaWieza(semid_turysta_wieza, semid_przewodnik_wieza, wiek, id_turysta);
				TurystaMost(semid_turysta_most, semid_przewodnik_most, wiek, id_turysta);
				break;
		}
		
		// Koniec wycieczki
		printf(GRN "\n-------Koniec wycieczki-------\n\n" RESET);
		semafor_operacja(semid_wyjscie, -1);
		
	}else{ // Turysta jest vipem
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

void TurystaMost(int semid_turysta_most, int semid_przewodnik_most, int wiek, int id_turysta){
	int shm_id;
    SharedData *shm_ptr = shm_init(&shm_id);
	
	semafor_operacja(semid_turysta_most, -1); // Turysta czeka na sygnał od przewodnika
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
	semafor_operacja(semid_przewodnik_most, 1); // Turysta przekazuje sygnał gotowości od przewodnika
}

void TurystaWieza(int semid_turysta_wieza, int semid_przewodnik_wieza, int semid_wieza_limit, int wiek, int id_turysta){
	int shm_id;
    SharedData *shm_ptr = shm_init(&shm_id);
	
	semafor_operacja(semid_turysta_wieza, -1); // Turysta czeka na sygnał od przewodnika
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
	semafor_operacja(semid_przewodnik_wieza, 1); // Turysta przekazuje sygnał gotowości od przewodnika
}

void TurystaProm(int semid_turysta_prom, int semid_przewodnik_prom, int semid_przeplyniecie, int wiek, int id_turysta){
	int shm_id;
    SharedData *shm_ptr = shm_init(&shm_id);
	
	semafor_operacja(semid_turysta_prom, -1); // Turysta czeka na sygnał od przewodnika
	printf(BLU "\n-------Płynięcie promem-------\n\n" RESET);
			
	if(wiek<15){
		printf("[Turysta %d]: Wchodzę na prom pod opieką osoby dorosłej...\n", id_turysta);
	}else{
		printf("[Turysta %d]: Wchodzę na prom...\n", id_turysta);
	}
	
	sleep(2);
	printf("[Turysta %d]: Czekam na promie...\n", id_turysta);
	semafor_operacja(semid_przeplyniecie, -1);
	
	printf("[Turysta %d]: Przeplynąłem promem na drugą stronę\n", id_turysta);
	sleep(2);
	printf("[Turysta %d]: Czekam na resztę mojej grupy wraz z przewodnikiem\n", id_turysta);
	semafor_operacja(semid_przewodnik_prom, 1); // Turysta przekazuje sygnał gotowości od przewodnika
}
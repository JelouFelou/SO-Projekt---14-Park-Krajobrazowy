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
void VipMost(int IDkolejki, int wiek, int id_turysta, int typ_trasy, int semid_most_wchodzenie, int semid_most);
void VipWieza(int IDkolejki, int wiek, int id_turysta, int typ_trasy);
void VipProm(int IDkolejki, int wiek, int id_turysta, int typ_trasy, int semid_turysta_wchodzenie);

int numer=0;

int main() {
	srand((unsigned)time(NULL) ^ (getpid() << 16));
	struct komunikat kom;
	int IDkolejki;
	int semid_most_wchodzenie, semid_most, semid_turysta_wchodzenie;
	
	int id_turysta = getpid();
	int id_przewodnik = 0;
	int id_kasjer = 0;
	int vip = (rand() % 100 == 47);
	int typ_trasy = 0;
	int wiek = 0;
	int czas_trasa=0;
	
	
	while(1){
		if (!czy_istnieje(id_kasjer)) continue;
		else if (!czy_istnieje(id_turysta)) continue;
		else break;
	}
	// Inicjalizacja pamięci współdzielonej
	int shm_id;
    SharedData *shm_ptr = shm_init(&shm_id);
	
	// Jeśli turysta jest VIP-em – ustawiamy losowo trasę oraz wiek
	if(vip){
		typ_trasy = (rand() % 2) + 1;
		wiek = (rand() % 62) + 18;
	}
	// Dla zwykłego turysty dane (typ trasy i wiek) zostaną nadane przez kasjera
	
	
	if(shm_ptr->liczba_turystow==N){
		printf("Osiągnięto już limit turystów danego dnia\n");
		exit(1);
	}else{
		shm_ptr->liczba_turystow++;
	}
	
	signal(SIGTERM,przedwczesne_wyjscie);
	
	// Ten napis zostanie wypisany tylko raz
	if(shm_ptr->turysta_istnieje==0){
		shm_ptr->turysta_istnieje=1;
		printf(GRN "-------Symulacja parku krajobrazowego - Turysta-------\n\n" RESET);
	}

    // Pobieranie ID kolejki komunikatów
	key_t key_kolejka = ftok(".", 99);
    if ((IDkolejki = msgget(key_kolejka, IPC_CREAT | 0600)) == -1) {
        perror("msgget() błąd");
        exit(1);
    }
	
	// Odpowiedzialne za kontrolę dostępu do atrakcji przez innych przewodników
	key_t key_most = ftok(".", 200);
	if ((semid_most = semget(key_most, 1, IPC_CREAT | 0600)) == -1) {
		perror("Błąd przy tworzeniu semafora mostu");
		exit(1);
	}
	key_t key_turysta_wchodzenie = ftok(".", 203);
	if ((semid_turysta_wchodzenie = semget(key_turysta_wchodzenie, 1, IPC_CREAT | 0600)) == -1) {
		perror("Błąd przy tworzeniu semafora turysta_wchodzenie");
		exit(1);
	}
	key_t key_most_wchodzenie = ftok(".", 205);
	if ((semid_most_wchodzenie = semget(key_most_wchodzenie, 1, IPC_CREAT | 0600)) == -1) {
		perror("Błąd przy tworzeniu semafora most_wchodzenie");
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
			printf("[Turysta %d] odbiera %s\n", id_turysta, kom.mtext);
			sscanf(kom.mtext, "bilet na trasę %d dla osoby z wiekiem %d. Ma pójść do przewodnika %d, jego numer to %d", &typ_trasy, &wiek, &id_przewodnik, &numer);
		}
		
	// ---- Przewodnik ----
		sleep(2);
		printf("[Turysta %d] czeka na rozpoczęcie oprowadzania...\n", id_turysta);
		
		// Turysta czeka aż przewodnik zacznie wycieczkę
		kom.mtype = id_turysta;
		if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, id_turysta, 0) == -1) {
			perror("msgrcv failed");
		} else {
			printf(GRN"[Turysta %d] jest podekscytowany zwiedzaniem parku!\n"RESET, id_turysta);
		}
	
		// Wycieczka zależna od typu trasy
		switch(typ_trasy){
			case(1):
				TurystaMost(IDkolejki, id_przewodnik, wiek, id_turysta);
				TurystaWieza(IDkolejki, id_przewodnik, wiek, id_turysta);
				TurystaProm(IDkolejki, id_przewodnik, wiek, id_turysta);
				break;
			case(2):
				TurystaProm(IDkolejki, id_przewodnik, wiek, id_turysta);
				TurystaWieza(IDkolejki, id_przewodnik, wiek, id_turysta);
				TurystaMost(IDkolejki, id_przewodnik, wiek, id_turysta);
				break;
		}
		
		//X. Koniec wycieczki
		//printf(GRN "\n-------Koniec wycieczki-------\n\n" RESET);
		if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, id_turysta, 0) == -1){
			perror("msgrcv failed");
		}
		
	} 
	else { // Turysta jest vipem
		printf(YEL"[Turysta %d] jest vipem i decyduje się pójść trasą %d\n"RESET, id_turysta, typ_trasy);
		shm_ptr->turysci_w_grupie++;
		sleep(2);
		czas_trasa = rand() % 2 + 1;
		switch(typ_trasy){
			case(1):
				VipMost(IDkolejki, wiek, id_turysta, typ_trasy, semid_most_wchodzenie, semid_most);
				VipWieza(IDkolejki, wiek, id_turysta, typ_trasy);
				VipProm(IDkolejki, wiek, id_turysta, typ_trasy, semid_turysta_wchodzenie);
				break;
			case(2):
				VipProm(IDkolejki, wiek, id_turysta, typ_trasy, semid_turysta_wchodzenie);
				VipWieza(IDkolejki, wiek, id_turysta, typ_trasy);
				VipMost(IDkolejki, wiek, id_turysta, typ_trasy, semid_most_wchodzenie, semid_most);
				break;
		}
	}
	
	int pamiatki = (rand() % 10 == 5);
	if(pamiatki){
		printf("[Turysta %d] decyduje kupić jakąś pamiątkę\n", id_turysta);
		sleep(2);
		int kupno_pamiatki = (rand() % 5) + 1;
		switch(kupno_pamiatki){
			case(1):
				printf("[Turysta %d] kupuje kulę śnieżną\n", id_turysta);
				break;
			case(2):
				printf("[Turysta %d] kupuje magnes\n", id_turysta);
				break;
			case(3):
				printf("[Turysta %d] kupuje kalendarz\n", id_turysta);
				break;
			case(4):
				printf("[Turysta %d] kupuje kubek\n", id_turysta);
				break;
			case(5):
				printf("[Turysta %d] kupuje zawieszkę\n", id_turysta);
				break;
			default:
				printf("[Turysta %d] rezygnuje z zakupów\n", id_turysta);
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

// --- Funkcje odpowiedzialne za wycieczkę turysty
void TurystaMost(int IDkolejki, int id_przewodnik, int wiek, int id_turysta){
	int shm_id;
    SharedData *shm_ptr = shm_init(&shm_id);
	struct komunikat kom;
	
	float czas = 0;
	
//1. Turysta czeka na sygnał od przewodnika
	if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, id_turysta, 0) == -1) {
		perror("msgrcv failed");
	}else{
		sscanf(kom.mtext, "OK %f", &czas);
	}
	
// --- Rozpoczęcie mostu wiszącego
	//printf(GRN "\n-------Most Wiszący-------\n\n" RESET);
	sleep(1);
	if(wiek<15){
		printf("[Turysta %d]: Wchodzę na most pod opieką osoby dorosłej...\n", id_turysta);
	}else{
		printf("[Turysta %d]: Wchodzę na most...\n", id_turysta);
	}
	sleep(czas);
	printf("[Turysta %d]: Podziwiam widoki\n", id_turysta);
	sleep(czas);
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
    SharedData *shm_ptr = shm_init(&shm_id);
	struct komunikat kom;
	
	float czas = 0;
	
//1. Turysta czeka na sygnał od przewodnika
	if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, id_turysta, 0) == -1) {
        perror("msgrcv failed (Wieża)");
    }else{
		sscanf(kom.mtext, "OK %f", &czas);
	}
	
// --- Rozpoczęcie wchodzenia na wieżę widokową
	//printf(GRN "\n-------Wieża Widokowa-------\n\n" RESET);
	// Turysta wchodzi na wieżę jedną klatką schodową
	if(wiek<=5){
		printf("[Turysta %d]: Czekam pod wieżą aż reszta grupy zejdzie...\n", id_turysta);
	}else if(wiek<15){
		printf("[Turysta %d]: Wchodzę na wieżę pod opieką osoby dorosłej...\n", id_turysta);
	}else{
		printf("[Turysta %d]: Wchodzę na wieżę...\n", id_turysta);
	}
	
	if(shm_ptr->wieza_sygnal[numer] == 1){
		printf("[Turysta %d]: Natychmiastowo schodzi z wieży...\n", id_turysta);
		shm_ptr->liczba_osob_na_wiezy--;
	}
	
	//Turysta na wieży
	if(wiek>5){
		sleep(czas);
		printf("[Turysta %d]: Wszedłem na wieżę...\n", id_turysta);
		if(shm_ptr->wieza_sygnal[numer] == 1){
			printf("[Turysta %d]: Natychmiastowo schodzi z wieży...\n", id_turysta);
			shm_ptr->liczba_osob_na_wiezy--;
		}else{
			sleep(1);
			printf("[Turysta %d]: Podziwia widoki...\n", id_turysta);
			if(shm_ptr->wieza_sygnal[numer] == 1){
				printf("[Turysta %d]: Natychmiastowo schodzi z wieży...\n", id_turysta);
				shm_ptr->liczba_osob_na_wiezy--;
			}else{
				sleep(2);
			// Turysta schodzi z wieży drugą klatką schodową
				if(shm_ptr->wieza_sygnal[numer] == 1){
					printf("[Turysta %d]: Natychmiastowo schodzi z wieży...\n", id_turysta);
					shm_ptr->liczba_osob_na_wiezy--;
				}else{
					printf("[Turysta %d]: Zchodzi z wieży...\n", id_turysta);
					if(shm_ptr->wieza_sygnal[numer] == 1){
						printf("[Turysta %d]: Natychmiastowo schodzi z wieży...\n", id_turysta);
						shm_ptr->liczba_osob_na_wiezy--;
					}else{
						sleep(czas);
						printf("[Turysta %d]: Zszedł z wieży i czeka na resztę...\n", id_turysta);
						shm_ptr->liczba_osob_na_wiezy--;
					}
				}
			}
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
    SharedData *shm_ptr = shm_init(&shm_id);
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
	//printf(BLU "\n-------Płynięcie promem-------\n\n" RESET);
			
	if(wiek<15){
		printf("[Turysta %d]: Wchodzę na prom pod opieką osoby dorosłej...\n", id_turysta);
	}else{
		printf("[Turysta %d]: Wchodzę na prom...\n", id_turysta);
	}
	sleep(2);
	printf("[Turysta %d]: Czekam na promie...\n", id_turysta);
//2. Turysta wchodzi na prom 
	if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, PROM + id_turysta + PROM_EXIT_OFFSET, 0) == -1) {
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


// --- Funkcje odpowiedzialne za wycieczkę vipa
void VipMost(int IDkolejki, int wiek, int id_turysta, int typ_trasy, int semid_most_wchodzenie, int semid_most){
	int shm_id;
    SharedData *shm_ptr = shm_init(&shm_id);
	
	float czas = rand() % 10 + 1;
	printf("[VIP | %d][Turysta %d]: Widzi most\n",typ_trasy, id_turysta);
	
	semafor_operacja(semid_most_wchodzenie, -1);
	if(shm_ptr->most_kierunek == 0){ // Most wolny, ustawiamy kierunek
		shm_ptr->most_kierunek = typ_trasy;
		printf("[VIP | %d][Turysta %d]: Droga wolna! Most jest już dostępny\n",typ_trasy, id_turysta);
		semafor_operacja(semid_most_wchodzenie, 1);
	}else if(shm_ptr->most_kierunek != typ_trasy){ // Most zajęty przez inną trasę, czekamy
		shm_ptr->czekajacy_przewodnicy_most++;
		semafor_operacja(semid_most_wchodzenie, 1);
		semafor_operacja(semid_most,-1);
		shm_ptr->most_kierunek=typ_trasy;
		printf("[VIP | %d][Turysta %d]: Droga jest już wolna! Most jest już dostępny\n",typ_trasy, id_turysta);
	}else if(shm_ptr->most_kierunek == typ_trasy){
		printf("[VIP | %d][Turysta %d]: Inna grupa porusza się tą samą trasą co ja, mogę wejść na ten most\n",typ_trasy, id_turysta);
		semafor_operacja(semid_most_wchodzenie, 1);
	}
	
	if(shm_ptr->liczba_osob_na_moscie <= X1){
		shm_ptr->liczba_osob_na_moscie++;
		printf("[VIP | %d][Turysta %d]: Wchodzę na most...\n",typ_trasy, id_turysta);
		sleep(czas);
		printf("[VIP | %d][Turysta %d]: Podziwiam widoki\n",typ_trasy, id_turysta);
		sleep(czas);
		printf("[VIP | %d][Turysta %d]: Dotarłem na koniec mostu...\n",typ_trasy, id_turysta);
		shm_ptr->liczba_osob_na_moscie--;
	}
	
	if(shm_ptr->liczba_osob_na_moscie==0 && shm_ptr->przewodnicy_most==0){
		shm_ptr->most_kierunek = 0;
		if (shm_ptr->czekajacy_przewodnicy_most > 0){
			semafor_operacja(semid_most,shm_ptr->czekajacy_przewodnicy_most);
			shm_ptr->czekajacy_przewodnicy_most=0;
		}
	}
	
	printf("[VIP | %d][Turysta %d]: Mogę w takim wypadku iść dalej\n",typ_trasy, id_turysta);
	shmdt(shm_ptr);
}

void VipWieza(int IDkolejki, int wiek, int id_turysta, int typ_trasy){
	int shm_id;
    SharedData *shm_ptr = shm_init(&shm_id);
	
	float czas = rand() % 10 + 1;
	
	printf("[VIP | %d][Turysta %d]: Widzę wieże...\n",typ_trasy, id_turysta);
	sleep(1);
	/*while(1){
		if(shm_ptr->liczba_osob_na_wiezy <= X2){
			shm_ptr->liczba_osob_na_wiezy++;*/
			printf("[VIP | %d][Turysta %d]: Wchodzę na wieżę...\n",typ_trasy, id_turysta);
			sleep(czas);
			printf("[VIP | %d][Turysta %d]: Wszedłem na wieżę...\n",typ_trasy, id_turysta);
			sleep(1);
			printf("[VIP | %d][Turysta %d]: Podziwia widoki...\n",typ_trasy, id_turysta);
			sleep(2);
			printf("[VIP | %d][Turysta %d]: Zchodzi z wieży...\n",typ_trasy, id_turysta);
			sleep(czas);
			printf("[VIP | %d][Turysta %d]: Zszedł z wieży...\n",typ_trasy, id_turysta);
			/*shm_ptr->liczba_osob_na_wiezy--;
			break;
		}else{
			printf("[VIP | %d][Turysta %d]: Wieża pełna (obecnie: %d osób). Czekam na zwolnienie miejsca...\n",typ_trasy,id_turysta, shm_ptr->liczba_osob_na_wiezy);
			sleep(5);
			continue;
		}
	}*/
	
	sleep(1);
	printf("[VIP | %d][Turysta %d]: Mogę w takim wypadku iść dalej\n",typ_trasy, id_turysta);
	shmdt(shm_ptr);
}

void VipProm(int IDkolejki, int wiek, int id_turysta, int typ_trasy, int semid_turysta_wchodzenie){
	int shm_id;
    SharedData *shm_ptr = shm_init(&shm_id);
	struct komunikat kom;
	
	int wydluzenie=0;
	
	float czas = rand() % 10 + 1;
	
	if(typ_trasy==1){
		shm_ptr->turysci_trasa_1++;
	}else if(typ_trasy==2){
		shm_ptr->turysci_trasa_2++;
	}
	printf("[VIP | %d][Turysta %d]: Widzi prom\n",typ_trasy, id_turysta);
	sleep(1);
	printf("[VIP | %d][Turysta %d]: Sprawdzam stan promu...\n",typ_trasy, id_turysta);
	
	semafor_operacja(semid_turysta_wchodzenie, -1);
	if(shm_ptr->prom_kierunek==0){
		shm_ptr->prom_kierunek=typ_trasy;
		printf("[VIP | %d][Turysta %d]: Prom znajduje się po mojej stronie: %d\n",typ_trasy, id_turysta, typ_trasy);
	}else{
		printf("[VIP | %d][Turysta %d]: Prom nie znajduje się po mojej stronie: %d\n",typ_trasy, id_turysta, typ_trasy);
	}
	semafor_operacja(semid_turysta_wchodzenie, 1);
	
	while(1){
		semafor_operacja(semid_turysta_wchodzenie, -1);
		
	// W wypadku gdy przejdzie przez semafor i typ trasy jest inny albo prom jest aktualnie niedostępny
		if(shm_ptr->prom_kierunek != typ_trasy || shm_ptr->prom_odplynal==1){
			semafor_operacja(semid_turysta_wchodzenie, 1);
			continue;
		}	
	//1. Odczytuje komunikat OK od promu
		if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, PROM + PROM_START_OFFSET, 0) == -1) {
			perror("msgrcv failed");
		}
		
		if (shm_ptr->prom_zajete < X3 && shm_ptr->prom_kierunek == typ_trasy){
		// Wysyłana jest informacja do promu, że turysta wchodzi na prom
			kom.mtype = PROM + PROM_ENTER_OFFSET;
			sprintf(kom.mtext, "[Turysta %d] chce wejść na prom %d",id_turysta, wydluzenie);
			msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
			
			if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, PROM + PROM_WELCOME_OFFSET, 0) == -1) {
				perror("msgrcv failed");
			} else {
			//Wysyłana jest informacja do turysty by wszedł na prom
				printf("[VIP | %d][Turysta %d]: Wchodzę na prom (miejsc zajętych: %d/%d) (1:%d 2:%d)\n",typ_trasy, id_turysta, shm_ptr->prom_zajete, X3, shm_ptr->turysci_trasa_1, shm_ptr->turysci_trasa_2);
				sleep(2);
				printf("[VIP | %d][Turysta %d]: Czekam na promie...\n",typ_trasy, id_turysta);
				sleep(1);
				semafor_operacja(semid_turysta_wchodzenie, 1); // Odblokujemy dostęp do pętli dla kolejnej grupy
				break;
			}
		} 	
		if(shm_ptr->prom_zajete == X3){
			continue;
		}
	}
	
	if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, PROM + id_turysta + PROM_EXIT_OFFSET, 0) == -1) {
        perror("msgrcv failed (Prom - PROM)");
    } else {
        printf("[VIP | %d][Turysta %d]: Przeplynąłem promem na drugą stronę\n", typ_trasy, id_turysta);
		sleep(1);
    }
	
	printf("[VIP | %d][Turysta %d]: Mogę w takim wypadku iść dalej\n",typ_trasy, id_turysta);
	shmdt(shm_ptr);
}
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <math.h>
#include "header.h"
#include "trasa.h"

#define SEM_KEY 'A'

void awaryjne_wyjscie(int);

int grupa[M];
int liczba_w_grupie = 0;
int IDkolejki, semid_wyjscie, semid_wycieczka;
int semid_most, semid_wieza, semid_prom;
int semid_turysta_most, semid_turysta_wieza, semid_turysta_prom;
int semid_przewodnik_most, semid_przewodnik_wieza, semid_przewodnik_prom;


int main() {
	if(X1>=M){
		printf("X1 musi być mniejsze od M, aktualnie M = %d\n",M);
		return 0;
	}
	if(X2>=(2*M)){
		printf("X2 musi być mniejsze od 2M, aktualnie 2M = %d\n",2*M);
		return 0;
	}
	if(X3>=(1.5*M)){
		printf("X1 musi być mniejsze od 1.5M, aktualnie 1.5M = %.2f\n",1.5*M);
		return 0;
	}
	
	struct komunikat kom;
	int id_przewodnik = getpid();
	int id_turysta, typ_trasy;
	int wyczekuje = 1;
	
	key_t key_kolejka, key_semafor_wyjscie, key_semafor_wycieczka;
	key_t key_most, key_wieza, key_prom;
	key_t key_turysta_most, key_turysta_wieza, key_turysta_prom;
	key_t key_przewodnik_most, key_przewodnik_wieza, key_przewodnik_prom;
	
	
	printf(GRN "-------Symulacja parku krajobrazowego - Przewodnik %d-------\n\n" RESET,id_przewodnik);
	
	// Pobieranie ID kolejki komunikatów
	key_kolejka = ftok(".", 98);
    if ((IDkolejki = msgget(key_kolejka, IPC_CREAT | 0666)) == -1) {
        perror("msgget() błąd");
        exit(1);
    }
	
	// Koniec wycieczki
	key_semafor_wyjscie = ftok(".", 100);
    if ((semid_wyjscie = semget(key_semafor_wyjscie, 1, IPC_CREAT | 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	
	// Odpowiedzialny za informacje o rozpoczęciu wycieczki
	key_semafor_wycieczka = ftok(".", 101);
	if ((semid_wycieczka = semget(key_semafor_wycieczka, 1, IPC_CREAT | 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	
	// Odpowiedzialne za sterowanie turystą podczas pobytu na moście, wieży i promie
	key_turysta_most = ftok(".", 102);
	if ((semid_turysta_most = semget(key_turysta_most, 1, IPC_CREAT | 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	key_turysta_wieza = ftok(".", 103);
	if ((semid_turysta_wieza = semget(key_turysta_wieza, 1, IPC_CREAT | 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	key_turysta_prom = ftok(".", 104);
	if ((semid_turysta_prom = semget(key_turysta_prom, 1, IPC_CREAT | 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	
	// Odpowiedzialne za sterowanie przewodnikiem podczas pobytu na moście, wieży i promie
	key_przewodnik_most = ftok(".", 105);
	if ((semid_przewodnik_most = semget(key_przewodnik_most, 1, IPC_CREAT | 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	key_przewodnik_wieza = ftok(".", 106);
	if ((semid_przewodnik_wieza = semget(key_przewodnik_wieza, 1, IPC_CREAT | 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	key_przewodnik_prom = ftok(".", 107);
	if ((semid_przewodnik_prom = semget(key_przewodnik_prom, 1, IPC_CREAT | 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	
	// Odpowiedzialne za kontrolę dostępu do atrakcji przez innych przewodników
	key_most = ftok(".", 200);
	if ((semid_most = semget(key_most, 1, IPC_CREAT | 0666)) == -1) {
		perror("Błąd przy tworzeniu semafora mostu");
		exit(1);
	}
	key_wieza = ftok(".", 201);
	if ((semid_wieza = semget(key_wieza, 1, IPC_CREAT | 0666)) == -1) {
		perror("Błąd przy tworzeniu semafora wieży");
		exit(1);
	}
	key_prom = ftok(".", 202);
	if ((semid_prom = semget(key_prom, 1, IPC_CREAT | 0666)) == -1) {
		perror("Błąd przy tworzeniu semafora promu");
		exit(1);
	}
	
	
	union semun arg;
	// Inicjalizacja semaforów
    arg.val = 0;
		semctl(semid_wyjscie, 0, SETVAL, arg);
		semctl(semid_wycieczka, 0, SETVAL, arg);
		semctl(semid_turysta_most, 0, SETVAL, arg);
		semctl(semid_turysta_wieza, 0, SETVAL, arg);
		semctl(semid_turysta_prom, 0, SETVAL, arg);
		semctl(semid_przewodnik_most, 0, SETVAL, arg);
		semctl(semid_przewodnik_wieza, 0, SETVAL, arg);
		semctl(semid_przewodnik_prom, 0, SETVAL, arg);
	arg.val = X1;
		semctl(semid_most, 0, SETVAL, arg);
	arg.val = X2;
		semctl(semid_wieza, 0, SETVAL, arg);
	arg.val = X3;
		semctl(semid_prom, 0, SETVAL, arg);
		
	
	// Po nacisnieciu przez uzytkownika CTRL+C wywoluje sie funkcja awaryjne_wyjscie()
	signal(SIGINT,awaryjne_wyjscie); 
	

    while (1) {
		if(wyczekuje == 1){ // Wyświetli tylko raz | wyczekuje turystę zamiast za każdym razem jak turysta dołączy
			printf(YEL "[Przewodnik %d] wyczekuje turystów\n" RESET, id_przewodnik);
			wyczekuje=0;
		}
		
        if (msgrcv(IDkolejki, &kom, MAX, PRZEWODNIK, 0) == -1) {
            perror("msgrcv failed");
            continue;
        }
        
        sscanf(kom.mtext, "%d %d", &id_turysta, &typ_trasy);

        printf(">>>[Turysta %d] dołącza do trasy %d\n", id_turysta, typ_trasy);
        grupa[liczba_w_grupie++] = id_turysta;

        if (liczba_w_grupie == M) {
			sleep(2);
            printf(GRN"\n[Przewodnik %d]: \"Grupa zapełniona! Oprowadzę was po trasie %d\"\n"RESET,id_przewodnik, typ_trasy);
            sleep(1);
			semafor_operacja(semid_wycieczka, M);
			
			// Inna trasa zależna od typ_trasy
			switch(typ_trasy) {
				case 1:
				printf("[Przewodnik %d]: Jesteśmy przy kasach\n", id_przewodnik);
				sleep(1);
				TrasaA(IDkolejki, semid_most, semid_turysta_most, semid_przewodnik_most, id_przewodnik, grupa, liczba_w_grupie);
				sleep(1);
				TrasaB(IDkolejki, semid_wieza, id_przewodnik, grupa, liczba_w_grupie);
				sleep(1);
				TrasaC(IDkolejki, semid_prom, id_przewodnik, grupa, liczba_w_grupie);
				sleep(1);
				printf("[Przewodnik %d]: Wróciliśmy do kas\n", id_przewodnik);
				break;
			case 2:
				printf("[Przewodnik %d]: Jesteśmy przy kasach\n", id_przewodnik);
				sleep(1);
				//TrasaC(IDkolejki, semid_prom, id_przewodnik, grupa, liczba_w_grupie);
				//TrasaB(IDkolejki, semid_wieza, id_przewodnik, grupa, liczba_w_grupie);
				//TrasaA(IDkolejki, semid_most, id_przewodnik, grupa, liczba_w_grupie);
				printf("[Przewodnik %d]: Wróciliśmy do kas\n", id_przewodnik);
				break;
			}

            char lista_wychodzących[MAX] = ""; // Inicjalizacja pustej listy

			// Zerowanie grupy przed każdorazowym użyciem
			for (int i = 0; i < M; i++) {
				grupa[i] = 0; // Upewnij się, że grupa jest początkowo pusta
			}

			for (int i = 0; i < M; i++) {
				if (grupa[i] != 0) {
					char temp[10];
					sprintf(temp, "%d,", grupa[i]);  // Konwersja ID turysty do stringa
					if (strlen(lista_wychodzących) + strlen(temp) < MAX) { // Sprawdzanie, czy pomieści się w buforze
						strcat(lista_wychodzących, temp); // Dodanie ID turysty do listy
					}
				}
			}

			// Sprawdzamy, czy lista nie jest pusta
			if (strlen(lista_wychodzących) > 0) {
				kom.mtype = KASJER; // Ustawienie typu komunikatu
				strcpy(kom.mtext, lista_wychodzących); // Kopiowanie listy do komunikatu
				if (msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0) == -1) {
					perror("msgsnd failed");
				}
			} else {
				sleep(rand() % 4 + 3);
				printf("Wszyscy turyści bezpiecznie dotarli do kasy.\n");
			}

			semafor_operacja(semid_wyjscie, M);

			// Resetowanie liczby turystów i stanu oczekiwania
			liczba_w_grupie = 0;
			wyczekuje = 1;
			printf("\n");
        }
    }
	return 0;
}

void awaryjne_wyjscie(int sig_n) {
    extern int grupa[M];  // Przypisanie grupy turystów z głównego programu
    extern int liczba_w_grupie;  // Liczba turystów w grupie
    extern int IDkolejki;  // ID kolejki komunikatów
    extern int semid_most, semid_wieza, semid_prom;
	extern int semid_wycieczka, semid_wyjscie;
    extern int semid_turysta_most, semid_turysta_wieza, semid_turysta_prom;
	extern int semid_przewodnik_most, semid_przewodnik_prom, semid_przewodnik_wieza;
    

    struct komunikat kom;
    char lista_wychodzacych[MAX] = "";

    printf(RED"\nWyjście awaryjne! Turystyka przerywana! Prosimy o powrót do kasy!\n"RESET);

	for (int i = 0; i < M; i++) {
        grupa[i] = 0;
    }

    // Tworzymy listę turystów, którzy muszą opuścić park
    for (int i = 0; i < liczba_w_grupie; i++) {
        if (grupa[i] != 0) {
            char temp[10];
            sprintf(temp, "%d,", grupa[i]);
            if (strlen(lista_wychodzacych) + strlen(temp) < MAX) {
                strcat(lista_wychodzacych, temp);
            }
        }
    }

    // Wysyłamy komunikat do kasy, że turyści wychodzą
    if (strlen(lista_wychodzacych) > 0) {
        kom.mtype = KASJER;
        strcpy(kom.mtext, lista_wychodzacych);
        if (msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0) == -1) {
            perror("msgsnd failed");
        }
    } else {
		printf(GRN"Wszyscy turyści zostali wysłani do kasy.\n"RESET);
	}
	
	// Zwolnienie miejsc w semaforach dla turystów, aby uniknąć blokady
    for (int i = 0; i < liczba_w_grupie; i++) {
        semafor_operacja(semid_most, 1); // Zwolnienie mostu
        semafor_operacja(semid_wieza, 1); // Zwolnienie wieży
        semafor_operacja(semid_prom, 1); // Zwolnienie promu
    }

    // Zwolnienie semaforów
    semafor_operacja(semid_turysta_most, M);
    semafor_operacja(semid_turysta_wieza, M);
	semafor_operacja(semid_turysta_prom, M);
	semafor_operacja(semid_przewodnik_most, M);
	semafor_operacja(semid_przewodnik_wieza, M);
	semafor_operacja(semid_przewodnik_prom, M);
    semafor_operacja(semid_wycieczka, M);
    semafor_operacja(semid_wyjscie, M);

    // Przywracamy stan początkowy grupy
    liczba_w_grupie = 0;  // Grupa została opróżniona
	for (int i = 0; i < M; i++) {
        grupa[i] = 0;
    }

    // Zamykamy proces po wykonaniu awaryjnego wyjścia
    exit(0);
}

#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "header.h"
#include "trasa.h"

#define SEM_KEY 'A'

int grupa[M];
int liczba_w_grupie = 0;
void awaryjne_wyjscie(int);
int IDkolejki, semid_wyjscie;
int semid_most, semid_wieza, semid_prom;
int semid_kierunek;

int main() {
	key_t key_kolejka, key_semafor_wyjscie;
	key_t key_most, key_wieza, key_prom, key_most_kierunek;
    
    struct komunikat kom;
    int id_przewodnik = getpid();
	int id_turysta, typ_trasy;
	int wyczekuje = 1;
	
	printf(GRN "-------Symulacja parku krajobrazowego - Przewodnik %d-------\n\n" RESET,id_przewodnik);
	
	// Tworzenie kolejki komunikatów
    key_kolejka = ftok(".", 98);
	key_semafor_wyjscie = ftok(".", 100);
	// Tworzenie semaforów trasy
	key_most = ftok(".", 200);
	key_wieza = ftok(".", 201);
	key_prom = ftok(".", 202);

    if ((IDkolejki = msgget(key_kolejka, IPC_CREAT | 0666)) == -1) {
        perror("msgget() błąd");
        exit(1);
    }
	
	// Tworzenie semaforów
    if ((semid_wyjscie = semget(key_semafor_wyjscie, 1, IPC_CREAT | 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	if ((semid_most = semget(key_most, 1, IPC_CREAT | 0666)) == -1) {
		perror("Błąd przy tworzeniu semafora mostu");
		exit(1);
	}
	if ((semid_wieza = semget(key_wieza, 1, IPC_CREAT | 0666)) == -1) {
		perror("Błąd przy tworzeniu semafora wieży");
		exit(1);
	}
	if ((semid_prom = semget(key_prom, 1, IPC_CREAT | 0666)) == -1) {
		perror("Błąd przy tworzeniu semafora promu");
		exit(1);
	}
	if ((semid_kierunek = semget(key_most_kierunek, 1, IPC_CREAT | 0666)) == -1) {
		perror("Błąd przy tworzeniu semafora kierunku mostu");
		exit(1);
	}
	
	// Inicjalizacja semafora wyjścia na 0 (blokada wyjścia)
    union semun arg;
    arg.val = 0;
    semctl(semid_wyjscie, 0, SETVAL, arg);
	arg.val = X1;
	semctl(semid_most, 0, SETVAL, arg);
	arg.val = X2;
	semctl(semid_wieza, 0, SETVAL, arg);
	arg.val = X3;
	semctl(semid_prom, 0, SETVAL, arg);
	arg.val = 1;
	semctl(semid_kierunek, 0, SETVAL, arg);
	
	signal(SIGINT,awaryjne_wyjscie); //po nacisnieciu przez uzytkownika CTRL+C wywoluje sie funkcja awaryjne_wyjscie()

    while (1) {
		if(wyczekuje == 1){ //wyświetli tylko raz, że wyczekuje turystę zamiast za każdym razem jak turysta dołączy
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
            printf(GRN"\n[Przewodnik %d]: \"Grupa zapełniona! Oprowadzę was po trasie %d\"\n"RESET,id_przewodnik, typ_trasy);
            sleep(1);
			
			// Inna trasa zależna od typ_trasy
			switch(typ_trasy) {
				case 1:
				printf("[Przewodnik %d]: Jesteśmy przy kasach\n", id_przewodnik);
				sleep(1);
				TrasaA(IDkolejki, semid_most, semid_kierunek, id_przewodnik, grupa, liczba_w_grupie);
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
				TrasaC(IDkolejki, semid_prom, id_przewodnik, grupa, liczba_w_grupie);
				TrasaB(IDkolejki, semid_wieza, id_przewodnik, grupa, liczba_w_grupie);
				TrasaA(IDkolejki, semid_most, semid_kierunek, id_przewodnik, grupa, liczba_w_grupie);
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

			struct sembuf v = {0, M, 0}; 
			semop(semid_wyjscie, &v, 1);

			// Resetowanie liczby turystów i stanu oczekiwania
			liczba_w_grupie = 0;
			wyczekuje = 1;
			printf("\n");
        }
    }
}

void awaryjne_wyjscie(int sig_n) {
    extern int grupa[M];  // Przypisanie grupy turystów z głównego programu
    extern int liczba_w_grupie;  // Liczba turystów w grupie
    extern int IDkolejki;  // ID kolejki komunikatów

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

    // Przywracamy stan początkowy grupy
    liczba_w_grupie = 0;  // Grupa została opróżniona

    // Zwolnienie semafora dla wyjścia turystów (ponieważ są już w drodze do kasy)
    key_t key_semafor_wyjscie = ftok(".", 100);  // Ponownie pobieramy semafor wyjścia
    int semid_wyjscie = semget(key_semafor_wyjscie, 1, 0666);
    if (semid_wyjscie == -1) {
        perror("semget() błąd");
        exit(1);
    }

    struct sembuf v = {0, M, 0};
    semop(semid_wyjscie, &v, 1);

    // Zamykamy proces po wykonaniu awaryjnego wyjścia
    exit(0);
}

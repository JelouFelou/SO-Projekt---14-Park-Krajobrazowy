#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <unistd.h>
#include <string.h>
#include "header.h"

int turysci_w_parku[N];  // Rejestracja turystów
int liczba_turystow = 0;

int main() {
	struct komunikat kom;
    int id_kasjer = getpid();
	
	int IDkolejki, semid_kasa;
	key_t key_kolejka, key_semafor_kasa;

	printf(GRN "-------Symulacja parku krajobrazowego - Kasjer %d-------\n\n" RESET,id_kasjer);

    // Tworzenie kolejki komunikatów
	key_kolejka = ftok(".", 98);
    if ((IDkolejki = msgget(key_kolejka, IPC_CREAT | 0666)) == -1) {
        perror("msgget() błąd");
        exit(1);
    }

    // Tworzenie semafora
	key_semafor_kasa = ftok(".", 99);
    if ((semid_kasa = semget(key_semafor_kasa, 1, IPC_CREAT | 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }

    // Inicjalizacja semafora kasy na wartość 1 (kasa wolna)
    union semun arg;
    arg.val = 1;
		semctl(semid_kasa, 0, SETVAL, arg);
	
	
    while (1) {
        // Oczekiwanie na komunikat od turysty
		printf(YEL "[Kasjer %d] wyczekuje turysty\n" RESET, id_kasjer);
		
		// Pobranie turysty z kolejki (FIFO)
        if (msgrcv(IDkolejki, &kom, MAX, KASJER, 0) == -1) {
            perror("msgrcv failed");
            continue;
        }

		// Wywołanie turysty do kasy
        int id_turysta = 0;
		if (strstr(kom.mtext, "[Turysta") != NULL) {
			// Wyciąganie PID z komunikatu w formacie "[Turysta XXXX]"
			char *pid_start = strchr(kom.mtext, ' ') + 1;  // Szukamy spacji po "Turysta"
			if (pid_start) {
				id_turysta = strtol(pid_start, NULL, 10);
			}
		}

		
        printf(GRN "[Kasjer %d] wzywa turystę %d do kasy\n" RESET, id_kasjer, id_turysta);
		turysci_w_parku[liczba_turystow++] = id_turysta; // Dodaje turystę do parku
		sleep(2);
		
        // Wysyłanie zgody na podejście
        kom.mtype = id_turysta;
        sprintf(kom.mtext, "Zapraszamy do kasy");
        msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
		
        // Odbiór komunikatu od turysty
        if (msgrcv(IDkolejki, &kom, MAX, KASJER, 0) == -1) {
            perror("msgrcv failed");
            continue;
        }

		// Pobranie typu trasy z komunikatu
		int typ_trasy = 0;
		if (strstr(kom.mtext, "trasę") != NULL) {
			sscanf(strstr(kom.mtext, "trasę") + 6, "%d", &typ_trasy);
		}
		int wiek = 0;
		if (strstr(kom.mtext, "wiek") != NULL) {
			sscanf(strstr(kom.mtext, "wiek") + 5, "%d", &wiek);
		}
		sleep(1);
		
		// Wydawanie biletu
		if(wiek<8){
			printf("[Kasjer %d] Dzieci poniżej 8 roku życia nie płacą za bilet\n\n", id_kasjer);
		}
		printf("[Kasjer %d] wydaje bilet na trasę %d turyście %d\n\n", id_kasjer, typ_trasy, id_turysta);
		
		kom.mtype = id_turysta;
        sprintf(kom.mtext, "bilet na trasę %d", typ_trasy);
        msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
		
		// Przekazuje turystę do przewodnika
		kom.mtype = PRZEWODNIK;
		sprintf(kom.mtext, "%d %d %d", id_turysta, typ_trasy, wiek);
		msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);

        sleep(2);
		
		semafor_operacja(semid_kasa, 1);
	}

    return 0;
}
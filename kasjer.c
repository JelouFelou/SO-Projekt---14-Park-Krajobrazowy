#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <unistd.h>
#include <string.h>
#include "header.h"

int main() {
    key_t key_kolejka, key_semafor;
    int IDkolejki, semid;
    struct komunikat kom;
    int id_kasjer = getpid();

	printf(GRN "-------Symulacja parku krajobrazowego - Kasjer %d-------\n\n" RESET,id_kasjer);

    // Tworzenie klucza do kolejki i semafora
    key_kolejka = ftok(".", 98);
    key_semafor = ftok(".", 99);

    // Tworzenie kolejki komunikatów
    if ((IDkolejki = msgget(key_kolejka, IPC_CREAT | 0666)) == -1) {
        perror("msgget() błąd");
        exit(1);
    }

    // Tworzenie semafora
    if ((semid = semget(key_semafor, 1, IPC_CREAT | 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }

    // Inicjalizacja semafora na wartość 1 (kasa wolna)
    union semun arg;
    arg.val = 1;
    semctl(semid, 0, SETVAL, arg);
	
    while (1) {
        // Oczekiwanie na komunikat od turysty
		printf(YEL "[Kasjer %d] Wyczekuje turysty\n" RESET, id_kasjer);
		
		// Pobranie turysty z kolejki (FIFO)
        if (msgrcv(IDkolejki, &kom, MAX, SERWER, 0) == -1) {
            perror("msgrcv failed");
            continue;
        }

		// Wywołanie turysty do kasy
        int id_turysta = strtol(kom.mtext + 9, NULL, 10);  // Wyciąga PID z "[Turysta XXXX]"
        printf(GRN "[Kasjer %d] Wzywa turystę %d do kasy\n" RESET, id_kasjer, id_turysta);
		sleep(2);
		
        // Wysyłanie zgody na podejście
        kom.mtype = id_turysta;
        sprintf(kom.mtext, "Zapraszamy do kasy");
        msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
		
        // Odbiór komunikatu od turysty
        if (msgrcv(IDkolejki, &kom, MAX, SERWER, 0) == -1) {
            perror("msgrcv failed");
            continue;
        }

		// Pobranie typu trasy z komunikatu
		int typ_trasy = 0;
		if (strstr(kom.mtext, "trasę") != NULL) {
			sscanf(strstr(kom.mtext, "trasę") + 6, "%d", &typ_trasy);
		}
		sleep(1);
		
		// Wydawanie biletu
        printf("[Kasjer %d] Wydaje bilet na trasę %d turyście %d\n\n", id_kasjer, typ_trasy, id_turysta);
        kom.mtype = id_turysta;
        sprintf(kom.mtext, "bilet na trasę %d", typ_trasy);
        msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
        sleep(2);
		
		struct sembuf v = {0, 1, 0};
        semop(semid, &v, 1);
	}

    return 0;
}
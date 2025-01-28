#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include "header.h"

#define SERWER 1  // typ komunikatu do serwera (kasjer)
char temp[15];

int main() {
    key_t key;
    int IDkolejki;
	int id_kasjer = getpid();
    struct komunikat kom;

	printf(GRN "-------Symulacja parku krajobrazowego - Kasjer %d-------\n\n" RESET,id_kasjer);

    // Tworzenie unikalnego klucza do kolejki
    key = ftok(".", 98);

    // Tworzenie kolejki
    if ((IDkolejki = msgget(key, IPC_CREAT | 0666)) == -1) {
        perror("msgget() błąd");
        exit(1);
    }

    while (1) {
        // Oczekiwanie na komunikat od turysty
		printf(YEL "[Kasjer %d] Wyczekuje turysty\n" RESET, id_kasjer);
		
		// Odbieranie turysty z kolejki
        if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, SERWER, 0) == -1) {
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
        msgsnd(IDkolejki, (struct msgbuf *)&kom, strlen(kom.mtext) + 1, 0);
		sleep(1);
		
        // Odbieranie szczegółów zakupu
        if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, SERWER, 0) == -1) {
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
        msgsnd(IDkolejki, (struct msgbuf *)&kom, strlen(kom.mtext) + 1, 0);
        sleep(2);
	}

    return 0;
}
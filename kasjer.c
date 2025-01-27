#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <unistd.h>
#include <string.h>
#include "header.h"

#define SERWER 1  // typ komunikatu do serwera (kasjer)

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
		printf("[Kasjer %d] Wyczekuje turysty\n",id_kasjer);
        msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, SERWER, 0);

        // Kasjer obsługujący turystę
        printf("[Kasjer %d] Obsługuje turystę: %s\n",id_kasjer, kom.mtext);
		
        // Wydawanie biletu (symulacja)
        sleep(2);
        printf("[Kasjer %d] Wydaje bilet na trasę turyście: %s\n",id_kasjer, kom.mtext);
        
        // Wysyłanie odpowiedzi do turysty
		printf("[Kasjer %d] Wysylanie... %d -> %s\n", id_kasjer,id_kasjer, kom.mtext);
        msgsnd(IDkolejki, (struct msgbuf *)&kom, strlen(kom.mtext) + 1, 0);
    }
    return 0;
}
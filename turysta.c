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
	int id_turysta = getpid();
    struct komunikat kom;
	
	printf(GRN "-------Symulacja parku krajobrazowego - Turysta %d-------\n\n" RESET,id_turysta);

    // Tworzenie unikalnego klucza do kolejki
    key = ftok(".", 98);

    // Tworzenie kolejki
    if ((IDkolejki = msgget(key, IPC_CREAT | 0666)) == -1) {
        perror("msgget() błąd");
        exit(1);
    }

    sprintf(kom.mtext, "[Turysta %d] chce kupić bilet", id_turysta);
    kom.mtype = SERWER;  // Typ komunikatu do kasjera

    // Wysyłanie komunikatu do kasjera
    printf("[Turysta %d] Wysłał komunikat do kasjera: %s\n",id_turysta, kom.mtext);
	msgsnd(IDkolejki, (struct msgbuf *)&kom, strlen(kom.mtext) + 1, 0);

    // Odbiór odpowiedzi od kasjera
    msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, id_turysta, 0);
    printf("[Turysta %d] Odebrano odpowiedź od kasjera: %s\n",id_turysta, kom.mtext);

    return 0;
}

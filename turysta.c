#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include "header.h"

int main() {
	srand(time(NULL));
	
    key_t key_kolejka, key_semafor, key_semafor_wyjscie;
    int IDkolejki, semid, semid_wyjscie;
    int id_turysta = getpid();
    int typ_trasy = /*(rand() % 2) + */1;
    struct komunikat kom;
	
	printf(GRN "-------Symulacja parku krajobrazowego - Turysta %d-------\n\n" RESET,id_turysta);

	// Tworzenie klucza do kolejki i semafora
    key_kolejka = ftok(".", 98);
    key_semafor = ftok(".", 99);
	key_semafor_wyjscie = ftok(".", 100);

    // Pobieranie ID kolejki komunikatów
    if ((IDkolejki = msgget(key_kolejka, IPC_CREAT | 0666)) == -1) {
        perror("msgget() błąd");
        exit(1);
    }

    // Pobieranie ID semafora
    if ((semid = semget(key_semafor, 1, 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	if ((semid_wyjscie = semget(key_semafor_wyjscie, 1, 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }

	printf(">>> Do parku wchodzi [Turysta %d]\n", id_turysta);
	sleep(1);
	printf("> [Turysta %d] podchodzi do kasy\n\n",id_turysta);
	
	// Próba zajęcia semafora (czeka, jeśli kasa zajęta)
    struct sembuf p = {0, -1, 0};
    semop(semid, &p, 1);
	
	// Zgłoszenie do kolejki
    sprintf(kom.mtext, "[Turysta %d] zgłasza się do kolejki", id_turysta);
    kom.mtype = KASJER;
    msgsnd(IDkolejki, (struct msgbuf *)&kom, strlen(kom.mtext) + 1, 0);

    // Oczekiwanie na wezwanie do kasy
    if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, id_turysta, 0) == -1) {
        perror("msgrcv failed");
    } else {
        printf("[Turysta %d] został wezwany do kasy\n", id_turysta);
    }
	
	// Komunikat do kasjera
    sprintf(kom.mtext, "[Turysta %d] chce kupić bilet na trasę %d",id_turysta, typ_trasy);
    kom.mtype = KASJER;
    printf("[Turysta %d] przekazuje kasjerowi, że chce kupić bilet na trasę %d\n", id_turysta, typ_trasy);
	msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);

    // Odbiór biletu
    if (msgrcv(IDkolejki, &kom, MAX, id_turysta, 0) == -1)  {
		perror("msgrcv failed");
	} else {
		printf("[Turysta %d] odbiera %s\n\n", id_turysta, kom.mtext);
	}
	
	sleep(2);
	printf("[Turysta %d] otrzymał bilet i idzie do przewodnika\n", id_turysta);
    
    kom.mtype = PRZEWODNIK;
    sprintf(kom.mtext, "%d %d", id_turysta, typ_trasy);
    msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);

    printf("[Turysta %d] czeka na rozpoczęcie oprowadzania\n", id_turysta);
    struct sembuf p2 = {0, -1, 0};  // Czeka na przewodnika
    semop(semid_wyjscie, &p2, 1);

    printf("[Turysta %d] opuszcza park po zakończeniu trasy\n", id_turysta);

    return 0;
}
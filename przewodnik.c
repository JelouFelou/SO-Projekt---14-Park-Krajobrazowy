#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <unistd.h>
#include <string.h>
#include "header.h"

int grupa[M];
int liczba_w_grupie = 0;

int main() {
	key_t key_kolejka, key_semafor_wyjscie;
    int IDkolejki, semid_wyjscie;
    struct komunikat kom;
    int id_przewodnik = getpid();
	int wyczekuje = 1;
	
	printf(GRN "-------Symulacja parku krajobrazowego - Przewodnik %d-------\n\n" RESET,id_przewodnik);
	
	// Tworzenie kolejki komunikatów
    key_kolejka = ftok(".", 98);
	key_semafor_wyjscie = ftok(".", 100);

    if ((IDkolejki = msgget(key_kolejka, IPC_CREAT | 0666)) == -1) {
        perror("msgget() błąd");
        exit(1);
    }
	
	// Tworzenie semafora wyjścia
    if ((semid_wyjscie = semget(key_semafor_wyjscie, 1, IPC_CREAT | 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	
	// Inicjalizacja semafora wyjścia na 0 (blokada wyjścia)
    union semun arg;
    arg.val = 0;
    semctl(semid_wyjscie, 0, SETVAL, arg);

    while (1) {
		if(wyczekuje == 1){
			printf(YEL "[Przewodnik %d] wyczekuje turystów\n" RESET, id_przewodnik);
			wyczekuje=0;
		}
		
        if (msgrcv(IDkolejki, &kom, MAX, PRZEWODNIK, 0) == -1) {
            perror("msgrcv failed");
            continue;
        }

        int id_turysta, typ_trasy;
        sscanf(kom.mtext, "%d %d", &id_turysta, &typ_trasy);

        printf(">>>[Turysta %d] dołącza do trasy %d\n", id_turysta, typ_trasy);
        grupa[liczba_w_grupie++] = id_turysta;

        if (liczba_w_grupie == M) {
            printf(GRN"\n[Przewodnik %d]: \"Grupa zapełniona! Oprowadzę was po trasie %d\"\n"RESET,id_przewodnik, typ_trasy);
            sleep(5);

            char lista_wychodzących[MAX] = "";
            for (int i = 0; i < M; i++) {
                char temp[10];
                sprintf(temp, "%d ", grupa[i]);
                strcat(lista_wychodzących, temp);
            }

            kom.mtype = KASJER;
            strcpy(kom.mtext, lista_wychodzących);
            msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
			
            struct sembuf v = {0, M, 0}; 
            semop(semid_wyjscie, &v, 1);

            liczba_w_grupie = 0;
			wyczekuje = 1;
			printf("\n");
        }
    }
}
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
	struct komunikat kom;
	int id_turysta = getpid();
    int typ_trasy = /*(rand() % 2) + */1;
	//int wiek = (rand() % 70) + 19;
	
    key_t key_kolejka, key_semafor_kasa, key_semafor_wyjscie, key_semafor_wycieczka;
    key_t key_turysta_most, key_turysta_wieza, key_turysta_prom;
	key_t key_przewodnik_most, key_przewodnik_wieza, key_przewodnik_prom;
	int IDkolejki, semid_kasa, semid_wyjscie, semid_wycieczka;
	int semid_turysta_most, semid_turysta_wieza, semid_turysta_prom;
	int semid_przewodnik_most, semid_przewodnik_wieza, semid_przewodnik_prom;
	
	
	printf(GRN "-------Symulacja parku krajobrazowego - Turysta %d-------\n\n" RESET,id_turysta);

    // Pobieranie ID kolejki komunikatów
	key_kolejka = ftok(".", 98);
    if ((IDkolejki = msgget(key_kolejka, 0666)) == -1) {
        perror("msgget() błąd");
        exit(1);
    }

    // Pobieranie ID semafora
	key_semafor_kasa = ftok(".", 99);
    if ((semid_kasa = semget(key_semafor_kasa, 1, 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	
	// Koniec wycieczki
	key_semafor_wyjscie = ftok(".", 100);
	if ((semid_wyjscie = semget(key_semafor_wyjscie, 1, 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	
	// Odpowiedzialny za informacje o rozpoczęciu wycieczki
	key_semafor_wycieczka = ftok(".", 101);
	if ((semid_wycieczka = semget(key_semafor_wycieczka, 1, 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	
	// Odpowiedzialne za sterowanie turystą podczas pobytu na moście, wieży i promie
	key_turysta_most = ftok(".", 102);
	if ((semid_turysta_most = semget(key_turysta_most, 1, 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	key_turysta_wieza = ftok(".", 103);
	if ((semid_turysta_wieza = semget(key_turysta_wieza, 1, 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	key_turysta_prom = ftok(".", 104);
	if ((semid_turysta_prom = semget(key_turysta_prom, 1, 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	
	// Odpowiedzialne za sterowanie przewodnikiem podczas pobytu na moście, wieży i promie
	key_przewodnik_most = ftok(".", 105);
	if ((semid_przewodnik_most = semget(key_przewodnik_most, 1, 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	key_przewodnik_wieza = ftok(".", 106);
	if ((semid_przewodnik_wieza = semget(key_przewodnik_wieza, 1, 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	key_przewodnik_prom = ftok(".", 107);
	if ((semid_przewodnik_prom = semget(key_przewodnik_prom, 1, 0666)) == -1) {
        perror("semget() błąd");
        exit(1);
    }
	
	
	// ---- Kasjer ----

	printf(">>> Do parku wchodzi [Turysta %d]\n", id_turysta);
	sleep(1);
	printf("> [Turysta %d] podchodzi do kasy\n\n",id_turysta);
	
	// Próba zajęcia semafora (czeka, jeśli kasa zajęta)
	semafor_operacja(semid_kasa, -1);
	
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
	
	// ---- Przewodnik ----
	
	sleep(2);
	printf("[Turysta %d] otrzymał bilet i idzie do przewodnika\n", id_turysta);

    printf("[Turysta %d] czeka na rozpoczęcie oprowadzania...\n", id_turysta);
	semafor_operacja(semid_wycieczka, -1);
	printf("[Turysta %d] jest podekscytowany zwiedzaniem parku!\n", id_turysta);
	
	switch(typ_trasy){
		case(1):
			// Most
			printf(GRN "\n-------Most Wiszący-------\n\n" RESET);
			semafor_operacja(semid_turysta_most, -1);
			sleep(rand() % 1 + 1);
			printf("[Turysta %d]: Wchodzę na most...\n", id_turysta);
			sleep(1);
			printf("[Turysta %d]: Podziwia widoki\n", id_turysta);
			sleep(rand() % 2 + 1);
			printf("[Turysta %d]: Dotarłem na koniec mostu...\n", id_turysta);
			semafor_operacja(semid_przewodnik_most, 1);
			
			// Wieza
			
			// Prom
			semafor_operacja(semid_wyjscie, -1);
			break;
		case(2):
			
			semafor_operacja(semid_wyjscie, -1);
			break;
	}
	
    printf("[Turysta %d] opuszcza park po zakończeniu trasy\n", id_turysta);

    return 0;
}
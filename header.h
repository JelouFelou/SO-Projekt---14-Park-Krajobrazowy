#ifndef HEADER_H
#define HEADER_H

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>

#define N 50 // maksymalna ilość osób w parku w ciągu dnia
#define K 4 // liczba kas
#define M 3 // wielkość grupy odprowadzanej przez przewodnika
#define P 4 // liczba przewodników
#define MAX 256
#define KASJER 1  // typ komunikatu do kasjera
#define PRZEWODNIK 2 // typ komunikatu do przewodnika

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define RESET "\x1B[0m"

struct komunikat {
    long mtype;
    char mtext[MAX];
};

// Struktura pamięci współdzielonej
typedef struct {
    int liczba_osob_na_moscie;
	int liczba_osob_na_wiezy;
	int liczba_osob_na_promie
	int most_kierunek;
} SharedData;

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

void semafor_operacja(int semid, int zmiana) {
    struct sembuf operacja = {0, zmiana, 0};
    if (semop(semid, &operacja, 1) == -1) {
        perror("Błąd semop");
        exit(1);
    }
}

#endif
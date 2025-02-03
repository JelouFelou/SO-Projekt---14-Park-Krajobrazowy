#ifndef HEADER_H
#define HEADER_H

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>

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

union semun {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
};

// Wykonuje operacje na semaforze, zmieniając jego wartość o 'zmiana'
void semafor_operacja(int semid, int zmiana) {
    struct sembuf operacja = {0, zmiana, 0};
    if (semop(semid, &operacja, 1) == -1) {
        perror("Błąd semop");
        exit(1);
    }
}

// Struktura pamięci współdzielonej
typedef struct {
    int liczba_osob_na_moscie;
	int liczba_osob_na_wiezy;
	int liczba_osob_na_promie;
	int most_kierunek;
	int czekajacy_przewodnicy;
} SharedData;

// Inicjalizacja Pamięci Współdzielonej
SharedData* shm_init(int* shm_id){
	// Tworzymy segment pamięci współdzielonej
    key_t key = ftok("header.h", 1);  // Tworzymy unikalny klucz
    *shm_id = shmget(key, sizeof(SharedData), IPC_CREAT | 0666);
    if (*shm_id == -1) {
        perror("shmget");
        exit(1);
    }

    SharedData *shm_ptr = (SharedData *)shmat(*shm_id, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("shmat");
        exit(1);
    }

    // Inicjalizowanie danych w pamięci współdzielonej
    if (shm_ptr->liczba_osob_na_moscie == 0) {
		shm_ptr->liczba_osob_na_moscie = 0;  // Zapobieganie zerowaniu liczby w wypadku dołączenia nowego przewodnika
	}
	return shm_ptr;
}


#endif
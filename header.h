#ifndef HEADER_H
#define HEADER_H

#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <errno.h>

#define N 10000 // maksymalna ilość osób w parku w ciągu dnia
#define M 10 // wielkość grupy odprowadzanej przez przewodnika
#define P 10 // liczba przewodników
#define X1 5   // Maksymalna liczba osób na moście (X1<M)
#define X2 18   // Maksymalna liczba osób na wieży (X2<2M)
#define X3 14   // Maksymalna liczba osób na promie (X3<1.5*M)

#define TMOST 5 // określony czas pokonania mostu
#define TWIEZA 5 // określony czas pokonania wiezy
#define TPROM 5 // określony czas pokonania rzeki

#define MAX 256 // maksymalna długość wiadomości
#define KASJER 1  // typ komunikatu do kasjera
#define PRZEWODNIK 2 // typ komunikatu do przewodnika
#define PROM 3 // typ komunikatu do promu

// Przesunięcia komunikatów
#define PROM_START_OFFSET 31000
#define PROM_EXIT_OFFSET 32000
#define PROM_READY_OFFSET 33000
#define PROM_ENTER_OFFSET 34000
#define PROM_WELCOME_OFFSET 35000

// Kolory
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

// Funkcja sprawdzająca czy dany proces istnieje
int czy_istnieje(int pid) {
	if (kill(pid, 0) == 0) {
		// Proces istnieje.
        return 1;
	}else{
        if (errno == ESRCH) {
			// Proces o tym PID nie istnieje, zostaje pominięty
			return 0;
		} else if (errno == EPERM) {
			// Proces o tym PID nie istnieje, zostaje pominięty (Proces nie ma odpowiednich uprawnień)
			return 0;
		} else {
			printf("Inny błąd\n");
			return 0;
		}
	}
}


// ---- Pamięć Współdzielona

// Struktura
typedef struct {
	int init;
	
	// Turysta
	int liczba_turystow;
	
	//Przewodnik
	int turysci_w_grupie;
	int ilosc_przewodnikow;
	int nr_przewodnika;
	
	// Most
	int liczba_osob_na_moscie;
	int most_kierunek;
	int czekajacy_przewodnicy_most;
	int przewodnicy_most;
	
	// Wieża
	int liczba_osob_na_wiezy;
	int wieza_sygnal[P];
	
	// Prom
	int turysci_trasa_1;
	int turysci_trasa_2;
	int prom_kierunek;
	int prom_zajete;
	int prom_odplynal;
	
	// Wypisywanie informacji o symulacji tylko raz
	int prom_istnieje;
	int przewodnik_istnieje;
	int turysta_istnieje;
	int kasjer_istnieje;
} SharedData;


// Inicjalizacja 
SharedData* shm_init(int* shm_id){
	// Tworzymy segment pamięci współdzielonej
    key_t key = ftok("header.h", 1);  // Tworzymy unikalny klucz
	
    *shm_id = shmget(key, sizeof(SharedData), IPC_CREAT | 0600);
    if (*shm_id == -1) {
        perror("shmget");
        exit(1);
    }

    SharedData *shm_ptr = (SharedData *)shmat(*shm_id, NULL, 0);
    if (shm_ptr == (void *)-1) {
        perror("shmat");
        exit(1);
    }	
	
	if(shm_ptr->init != 1){
		shm_ptr->init = 1;
		
		// Turysta
		shm_ptr->liczba_turystow=0;
	
		//Przewodnik
		shm_ptr->turysci_w_grupie=0;
		shm_ptr->ilosc_przewodnikow=0;
		shm_ptr->nr_przewodnika=1;
	
		// Most
		shm_ptr->liczba_osob_na_moscie=0;
		shm_ptr->most_kierunek=0;
		shm_ptr->czekajacy_przewodnicy_most=0;
		shm_ptr->przewodnicy_most=0;
	
		// Wieża
		shm_ptr->liczba_osob_na_wiezy=0;
		for(int i=0; i<P; i++){
			shm_ptr->wieza_sygnal[i]=0;
		}
	
		// Prom
		shm_ptr->turysci_trasa_1=0;
		shm_ptr->turysci_trasa_2=0;
		shm_ptr->prom_kierunek=0;
		shm_ptr->prom_zajete=0;
		shm_ptr->prom_odplynal=0;
	
		// Wypisywanie informacji o symulacji tylko raz
		shm_ptr->prom_istnieje=0;
		shm_ptr->przewodnik_istnieje=0;
		shm_ptr->turysta_istnieje=0;
		shm_ptr->kasjer_istnieje=0;
	}
	return shm_ptr;
}


void LiczbaTurysciTrasy(int typ_trasy, SharedData *shm_ptr) {
    if (typ_trasy == 1) {
        shm_ptr->turysci_trasa_1--;
    } else if (typ_trasy == 2) {
        shm_ptr->turysci_trasa_2--;
    }
}

#endif
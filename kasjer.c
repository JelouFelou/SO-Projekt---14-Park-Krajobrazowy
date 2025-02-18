#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <time.h>
#include "header.h"

void reset_pamieci_wspoldzielonej(int);
int sygnal=0;


int main() {
	srand((unsigned)time(NULL) ^ (getpid() << 16));
	struct komunikat kom;
    int id_kasjer = getpid();
	int id_przewodnik = 0;
	int id_turysta = 0;
	int typ_trasy = 0;
	int wiek = 0;
	
	int IDkolejki;
	key_t key_kolejka;
	int wyczekuje = 1;
	int proba = 0, max_proba = 3;
	int zaakceptowanie = 0;

	// Inicjalizacja pamięci współdzielonej
	int shm_id;
    SharedData *shm_ptr = shm_init(&shm_id);

	printf(GRN "-------Symulacja parku krajobrazowego - Kasjer %d-------\n\n" RESET,id_kasjer);

    // Tworzenie kolejki komunikatów
	key_kolejka = ftok(".", 99);
    if ((IDkolejki = msgget(key_kolejka, IPC_CREAT | 0600)) == -1) {
        perror("msgget() błąd");
        exit(1);
    }
	
	signal(SIGUSR1, reset_pamieci_wspoldzielonej);
	
	
// ---- Pętla kasjera ----
    while (1) {
        // Oczekiwanie na komunikat od turysty o typie KASJER (ogólne zgłoszenie)
		if(wyczekuje==1){
			printf(YEL "[Kasjer %d] wyczekuje turysty\n" RESET, id_kasjer);
			wyczekuje=0;
		}
		
	// ---- Turysta ----
		
		//1. Pobranie turysty z kolejki
        if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, KASJER, 0) == -1) {
			perror("msgrcv failed");
		} else {
			sscanf(kom.mtext, "[Turysta %d] zgłasza się do kolejki", &id_turysta);
			if (!czy_istnieje(id_turysta)) continue;
			printf("[Kasjer %d] Zaprasza następnego turystę %d do kasy\n", id_kasjer, id_turysta);
		}

		//2. Wywołanie turysty do kasy – wyciągamy PID turysty z komunikatu
	
		// Sprawdzamy czy turysta o tym PID nadal istnieje
		if (!czy_istnieje(id_turysta)) continue;
		
        // Wysyłanie zgody na podejście
        kom.mtype = id_turysta;
		sprintf(kom.mtext, "Zapraszamy do kasy - od %d", id_kasjer);
		msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
		
		//3. Odbiór komunikatu od turysty – tym razem turysta wysyła wiadomość na mtype równy PID kasjera
        if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, id_kasjer, 0) == -1) {
			perror("msgrcv failed");
		} else {
			printf("[Kasjer %d] Przyjmuje zamówienie od turysty %d\n", id_kasjer, id_turysta);
		}
		
		// Ustalenie informacji o turyście
		typ_trasy = (rand() % 2) + 1;
		wiek = (rand() % 80) + 1;
		sleep(1);
		
	// ---- Przewodnik ----
		
		//3.1. Przekazuje turystę do przewodnika
		kom.mtype = PRZEWODNIK;
		sprintf(kom.mtext, "%d %d %d %d", id_turysta, typ_trasy, wiek, id_kasjer);
		msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);

		//3.2 Próba przyjęcia turysty przez przewodnika
		proba = 0, max_proba = 3;
		while(proba < max_proba) {
			if (msgrcv(IDkolejki, &kom, MAX, id_kasjer, 0) == -1) {
				perror("msgrcv (PRZEWODNIK)");
				continue;
			} else {
				if (strncmp(kom.mtext, "OK", 2) == 0) {
					printf("[Kasjer %d] Otrzymał potwierdzenie od przewodnika dla turysty %d!\n", id_kasjer, id_turysta);
					sscanf(kom.mtext, "OK %d", &id_przewodnik);
					
				//4. Przekazanie biletu
					kom.mtype = id_turysta;
					sprintf(kom.mtext, "bilet na trasę %d dla osoby z wiekiem %d. Ma pójść do przewodnika %d", typ_trasy, wiek, id_przewodnik);
					msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
			
					printf("[Kasjer %d] Przekazuje turyście %d bilet na trasę %d\n", id_kasjer, id_turysta, typ_trasy);
					break;
				}
				else if (strncmp(kom.mtext, "NO", 2) == 0) {
					printf("[Kasjer %d] Otrzymał odrzucenie od przewodnika dla turysty %d, szukam następnego!\n", id_kasjer, id_turysta);
					
					proba++;
					if (proba < max_proba){
						kom.mtype = PRZEWODNIK;
						sprintf(kom.mtext, "%d %d %d %d", id_turysta, typ_trasy, wiek, id_kasjer);
						msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);	
					}
			
					if (proba == max_proba){ 
						typ_trasy = (typ_trasy == 1) ? 2 : 1;
						proba = 0;
						kom.mtype = PRZEWODNIK;
						sprintf(kom.mtext, "%d %d %d %d", id_turysta, typ_trasy, wiek, id_kasjer);
						msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
					}
				}
			}
		}
		proba = 0;
		zaakceptowanie = 0;
		wyczekuje = 1;
		typ_trasy = 0;
		id_przewodnik = 0;
		id_turysta = 0;
		wiek = 0;

        sleep(2);
	}

    return 0;
}

void reset_pamieci_wspoldzielonej(int sig){
	int shm_id;
    SharedData *shm_ptr = shm_get(&shm_id);
	
	sygnal=1;
	shm_ptr->liczba_osob_na_moscie = 0;
    shm_ptr->liczba_osob_na_wiezy = 0;
    shm_ptr->liczba_osob_na_promie = 0;
    shm_ptr->most_kierunek = 0;
    shm_ptr->prom_kierunek = 0;
    shm_ptr->prom_zajete = 0;
	shm_ptr->czekajacy_przewodnicy_most = 0;
    shm_ptr->czekajace_grupy_prom = 0;
    shm_ptr->przewodnicy_most = 0;
    shm_ptr->turysci_trasa_1 = 0;
	shm_ptr->turysci_trasa_2 = 0;
    shm_ptr->liczba_turystow = 0;
	shm_ptr->wieza_sygnal = 0;
	shm_ptr->ilosc_przewodnikow = 0;
	
printf(YEL"[DEBUG] Reset pamięci współdzielonej został wykonany.\n"RESET);

    shmdt(shm_ptr);
}
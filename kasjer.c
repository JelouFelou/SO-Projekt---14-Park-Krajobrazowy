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
	srand(time(NULL));
	struct komunikat kom;
    int id_kasjer = getpid();
	int id_przewodnik = 0;
	int typ_trasy = 0;
	int wiek = 0;
	
	int IDkolejki;
	key_t key_kolejka;
	int wyczekuje = 1;

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
        if (msgrcv(IDkolejki, &kom, MAX, KASJER, 0) == -1) {
            if(sygnal){
				sygnal=0;
				continue;
			}else{
				perror("msgrcv failed");
				continue;
			}
        }

		//2. Wywołanie turysty do kasy – wyciągamy PID turysty z komunikatu
        int id_turysta = 0;
		if (strstr(kom.mtext, "[Turysta") != NULL) {
			char *pid_start = strchr(kom.mtext, ' ') + 1;
			if (pid_start) {
				id_turysta = strtol(pid_start, NULL, 10);
			}
		}
	
		// Sprawdzamy czy turysta o tym PID nadal istnieje
		if (!czy_istnieje(id_turysta)) {
			continue;
		}
		
        // Wysyłanie zgody na podejście
        kom.mtype = id_turysta;
        sprintf(kom.mtext, "Zapraszamy do kasy - od %d",id_kasjer);
        msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
		
		//3. Odbiór komunikatu od turysty – tym razem turysta wysyła wiadomość na mtype równy PID kasjera
        if (msgrcv(IDkolejki, &kom, MAX, id_kasjer, 0) == -1) {
            if(sygnal){
				sygnal=0;
				continue;
			}else{
				perror("msgrcv failed");
				continue;
			}
        }
		
		// Ustalenie informacji o turyście
		typ_trasy = (rand() % 2) + 1;
		wiek = (rand() % 80) + 1;
		sleep(1);
		
		//4. Wysyłanie biletu – wiadomość kierowana do turysty
		if(wiek < 8){
			printf("[Kasjer %d] Dzieci poniżej 8 roku życia nie płacą za bilet\n\n", id_kasjer);
		}
		printf("[Kasjer %d] wydaje bilet na trasę %d turyście %d\n\n", id_kasjer, typ_trasy, id_turysta);
		
		kom.mtype = id_turysta;
        sprintf(kom.mtext, "bilet na trasę %d dla osoby z wiekiem %d", typ_trasy, wiek);
        msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
		
	// ---- Przewodnik ----
		
		// Przekazuje turystę do przewodnika
		kom.mtype = PRZEWODNIK;
		sprintf(kom.mtext, "%d %d %d %d", id_turysta, typ_trasy, wiek, id_kasjer);
		msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);

		// Próba przyjęcia turysty przez przewodnika
		int proba = 0, max_proba = 3;
		int zaakceptowanie = 0;
		while (proba < max_proba) {
            if (msgrcv(IDkolejki, &kom, MAX, id_kasjer, 0) == -1) {
                perror("msgrcv from PRZEWODNIK failed");
                continue;
            }
			
            // Jeżeli przewodnik zaakceptował turystę, wysyłamy potwierdzenie do kasy
            if (strncmp(kom.mtext, "OK", 2) == 0) {
				printf("[Kasjer %d] Otrzymał potwierdzenie od przewodnika dla turysty %d!\n", id_kasjer, id_turysta);
				zaakceptowanie = 1;
				sscanf(kom.mtext, "OK %d", &id_przewodnik);
				break;
			}else if (strncmp(kom.mtext, "REJECT", 6) == 0) {
				printf("[Kasjer %d] Otrzymał odrzucenie od przewodnika dla turysty %d, szukam następnego!\n", id_kasjer, id_turysta);
				proba++;
				if (proba < max_proba) {
					kom.mtype = PRZEWODNIK;
					sprintf(kom.mtext, "%d %d %d %d", id_turysta, typ_trasy, wiek, id_kasjer);
					if (msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0) == -1) {
						perror("msgsnd to PRZEWODNIK (retry) failed");
					}
				} else {
					printf("[Kasjer %d] Maksymalna liczba prób osiągnięta dla turysty %d.\n", id_kasjer, id_turysta);
					break;
				}
			}else{
				break;
			}
		}
		
		// Przewodnik zaakceptował turystę
		if (zaakceptowanie) {
			kom.mtype = id_turysta;
			sprintf(kom.mtext, "OK %d %d",typ_trasy, id_przewodnik);
			msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
		} else {
        // Nie udało się przydzielić przewodnika, wysyłamy REJECT z propozycją zmiany trasy do turysty.
			kom.mtype = id_turysta;
			typ_trasy = (typ_trasy == 1) ? 2 : 1;
			sprintf(kom.mtext, "REJECT %d",typ_trasy);
			msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
			
			printf("[Kasjer %d] Daje turyscie propozycję zmiany typu trasy, tak żeby przewodnik był w stanie oprowadzić pełną grupę.\n", id_kasjer);
			kom.mtype = PRZEWODNIK;
			sprintf(kom.mtext, "%d %d %d %d", id_turysta, typ_trasy, wiek, id_kasjer);
			if (msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0) == -1) {
				perror("msgsnd to PRZEWODNIK (retry) failed");
			}
		}
		
		wyczekuje=1;
        sleep(2);
	}

    return 0;
}

void reset_pamieci_wspoldzielonej(int sig){
	int shm_id;
    SharedData *shm_ptr = shm_init(&shm_id);
	
	sygnal=1;
	shm_ptr->liczba_osob_na_moscie = 0;
    shm_ptr->liczba_osob_na_wiezy = 0;
    shm_ptr->liczba_osob_na_promie = 0;
    shm_ptr->most_kierunek = 0;
    shm_ptr->prom_kierunek = 0;
    shm_ptr->prom_zajete = 0;
	shm_ptr->czekajacy_przewodnicy_most = 0;
    shm_ptr->czekajacy_przewodnicy_prom = 0;
    shm_ptr->przewodnicy_most = 0;
    shm_ptr->turysci_trasa_1 = 0;
	shm_ptr->turysci_trasa_2 = 0;
    shm_ptr->liczba_turystow = 0;
	shm_ptr->wieza_sygnal = 0;
	shm_ptr->ilosc_przewodnikow = 0;
	
printf(YEL"[DEBUG] Reset pamięci współdzielonej został wykonany.\n"RESET);

    shmdt(shm_ptr);
}
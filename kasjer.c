#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include "header.h"

void reset_pamieci_wspoldzielonej(int);
int sygnal=0;


int main() {
	struct komunikat kom;
    int id_kasjer = getpid();
	
	int IDkolejki, semid_kasa;
	key_t key_kolejka, key_semafor_kasa;
	int wyczekuje = 1;

	// Inicjalizacja pamięci współdzielonej
	int shm_id;
    SharedData *shm_ptr = shm_init(&shm_id);

	printf(GRN "-------Symulacja parku krajobrazowego - Kasjer %d-------\n\n" RESET,id_kasjer);

    // Tworzenie kolejki komunikatów
	key_kolejka = ftok(".", 98);
    if ((IDkolejki = msgget(key_kolejka, IPC_CREAT | 0600)) == -1) {
        perror("msgget() błąd");
        exit(1);
    }

    // Tworzenie semafora kasy
	key_semafor_kasa = ftok(".", 99);
    if ((semid_kasa = semget(key_semafor_kasa, 1, IPC_CREAT | 0600)) == -1) {
        perror("semget() błąd");
        exit(1);
    }

    // Inicjalizacja semafora kasy na wartość 1 (kasa wolna)
    union semun arg;
    arg.val = P;
		semctl(semid_kasa, 0, SETVAL, arg);
	
	signal(SIGUSR1, reset_pamieci_wspoldzielonej);
	
    while (1) {
        // Oczekiwanie na komunikat od turysty o typie KASJER (ogólne zgłoszenie)
		printf(YEL "[Kasjer %d] wyczekuje turysty\n" RESET, id_kasjer);
		
		// Pobranie turysty z kolejki
        if (msgrcv(IDkolejki, &kom, MAX, KASJER, 0) == -1) {
            if(sygnal){
				sygnal=0;
				continue;
			}else{
				perror("msgrcv failed");
				continue;
			}
        }

		// Wywołanie turysty do kasy – wyciągamy PID turysty z komunikatu
        int id_turysta = 0;
		if (strstr(kom.mtext, "[Turysta") != NULL) {
			char *pid_start = strchr(kom.mtext, ' ') + 1;
			if (pid_start) {
				id_turysta = strtol(pid_start, NULL, 10);
			}
		}

		
        printf(GRN "[Kasjer %d] wzywa turystę %d do kasy\n" RESET, id_kasjer, id_turysta);
		sleep(2);
		
        // Wysyłanie zgody na podejście
        kom.mtype = id_turysta;
        sprintf(kom.mtext, "Zapraszamy do kasy - od %d",id_kasjer);
        msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
		
		// Odbiór komunikatu od turysty – tym razem turysta wysyła wiadomość na mtype równy PID kasjera
        if (msgrcv(IDkolejki, &kom, MAX, id_kasjer, 0) == -1) {
            if(sygnal){
				sygnal=0;
				continue;
			}else{
				perror("msgrcv failed");
				continue;
			}
        }
		// Pobranie typu trasy oraz wieku z komunikatu
		int typ_trasy = 0, wiek = 0;
		sscanf(kom.mtext, "[Turysta %d](wiek %d) chce kupić bilet na trasę %d", &id_turysta, &wiek, &typ_trasy);
		sleep(1);
		
		if(typ_trasy==0){
			typ_trasy=1;
		}
		
		// Wydawanie biletu
		if(wiek < 8){
			printf("[Kasjer %d] Dzieci poniżej 8 roku życia nie płacą za bilet\n\n", id_kasjer);
		}
		printf("[Kasjer %d] wydaje bilet na trasę %d turyście %d\n\n", id_kasjer, typ_trasy, id_turysta);
		
		// Wysyłanie biletu – wiadomość kierowana do turysty
		kom.mtype = id_turysta;
        sprintf(kom.mtext, "bilet na trasę %d", typ_trasy);
        msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
		
		// Przekazuje turystę do przewodnika
		kom.mtype = PRZEWODNIK;
		sprintf(kom.mtext, "%d %d %d %d", id_turysta, typ_trasy, wiek, id_kasjer);
		msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);

		int proba = 0, max_proba = P;
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
		
		if (zaakceptowanie) {
        // Jeśli przewodnik zaakceptował turystę, wysyłamy potwierdzenie do turysty.
			kom.mtype = id_turysta;
			sprintf(kom.mtext, "OK %d"typ_trasy);
			msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
		} else {
        // Jeśli nie udało się przydzielić przewodnika, wysyłamy REJECT z propozycją zmiany trasy do turysty.
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

        sleep(2);
		//semafor_operacja(semid_kasa, 1);
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
    shm_ptr->klatka_pierwsza = 0;
    shm_ptr->klatka_druga = 0;
    shm_ptr->turysci_trasa_1 = 0;
	shm_ptr->turysci_trasa_2 = 0;
    shm_ptr->liczba_turystow = 0;
	shm_ptr->wieza_sygnal = 0;
	shm_ptr->ilosc_przewodnikow = 0;
	
	printf(YEL"Reset pamięci współdzielonej został wykonany.\n"RESET);

    shmdt(shm_ptr);
}
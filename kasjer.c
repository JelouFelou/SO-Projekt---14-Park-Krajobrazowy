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

SharedData *global_shm_ptr = 0;

void przedwczesne_wyjscie(int);
int sygnal=0;


int main() {
	srand((unsigned)time(NULL) ^ (getpid() << 16));
	struct komunikat kom;
    int id_kasjer = getpid();
	int id_przewodnik = 0;
	int id_turysta = 0;
	int typ_trasy = 0;
	int wiek = 0;
	int numer = 0;
	int rola_kasjera=0;
	
	int IDkolejki;
	key_t key_kolejka;
	int wyczekuje = 1;
	int proba = 0, max_proba = 3;
	int zaakceptowanie = 0;

	// Inicjalizacja pamięci współdzielonej
	int shm_id;
    SharedData *shm_ptr = shm_init(&shm_id);
	global_shm_ptr = shm_ptr;

	if(N<=0 || M<=0 || P<=0 || X1<=0 || X2<=0 || X3<=0 || KLATKA<=0 || MAX<=0){
		printf("N, M, P, X1, X2, X3, KLATKA czy MAX nie mogą być mniejsze bądź równe 0\n");
		exit(1);
	}

	if(shm_ptr->kasjer_istnieje==0){
		shm_ptr->kasjer_istnieje=1;
		printf(GRN "-------Symulacja parku krajobrazowego - Kasjer-------\n\n" RESET);
	}
	
	if(shm_ptr->kasjer_glowny==0){
		shm_ptr->kasjer_glowny=1;
		rola_kasjera=1;
		printf("---9:00: Park jest otwarty i gotowy do przyjmowania turystów! ---\n");
		//time_t simulation_start = time(NULL);
	}
	
    // Tworzenie kolejki komunikatów
	key_kolejka = ftok(".", 99);
    if ((IDkolejki = msgget(key_kolejka, IPC_CREAT | 0600)) == -1) {
        perror("msgget() błąd");
        exit(1);
    }
	
	signal(SIGTERM, przedwczesne_wyjscie);
	
	
	
// ---- Pętla kasjera ----
    while (1) {
        // Oczekiwanie na komunikat od turysty o typie KASJER (ogólne zgłoszenie)
		if(wyczekuje==1){
			printf(YEL "[Kasjer %d] wyczekuje turysty\n" RESET, id_kasjer);
			wyczekuje=0;
		}
		
	// ---- Turysta ----
		
		/*if (now - simulation_start >= TK) {
            printf(RED "[Kasjer %d] Park zamknięty. Koniec przyjmowania zgłoszeń.\n" RESET,id_kasjer);
            break;  // lub dokonaj odpowiednich czynności zamykających symulację
        }*/
		
		if(rola_kasjera==1 && shm_ptr->turysta_opuszcza_park==shm_ptr->liczba_turystow && shm_ptr->turysta_opuszcza_park!=0 && shm_ptr->liczba_turystow!=0){
			sleep(1);
			printf("---18:00: Park zamyka się. Dziękujemy za odwiedziny w parku! ---\n");
			shm_ptr->kasjer_glowny=2;
			system("killall przewodnik");
			system("killall prom");
			system("killall kasjer");
		}
		
		//1. Pobranie turysty z kolejki
        if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, KASJER, IPC_NOWAIT) == -1) {
			if (errno == ENOMSG) {
				//
			} else {
				perror("msgrcv failed");
			}
		} else {
			sscanf(kom.mtext, "[Turysta %d] zgłasza się do kolejki", &id_turysta);
			if (!czy_istnieje(id_turysta)) continue;
			printf("[Kasjer %d] Zaprasza następnego turystę %d do kasy\n", id_kasjer, id_turysta);
		
		//time_t now = time(NULL);
		

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
		
			if(wiek < 8){
				printf("[Kasjer %d] Turyści poniżej 8 roku życia nie płacą za bilet\n", id_kasjer);
			}
		
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
					sscanf(kom.mtext, "OK %d %d", &id_przewodnik, &numer);
					
				//4. Przekazanie biletu
					kom.mtype = id_turysta;
					sprintf(kom.mtext, "bilet na trasę %d dla osoby z wiekiem %d. Ma pójść do przewodnika %d, jego numer to %d", typ_trasy, wiek, id_przewodnik, numer);
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
		}
        sleep(2);
	}

    return 0;
}

void przedwczesne_wyjscie(int sig){
	int id_kasjer = getpid();
	
	if(global_shm_ptr->kasjer_glowny==2){
		printf("[Kasjer %d] opuszcza park po całym dniu ciężkiej pracy\n", id_kasjer);
	}else{
		printf("[Kasjer %d] przedwcześnie opuszcza park\n", id_kasjer);
	}
	
	// Turysta
	global_shm_ptr->liczba_turystow=0;
	global_shm_ptr->turysta_opuszcza_park=0;
	global_shm_ptr->liczba_vipow=0;
	
	//Przewodnik
	global_shm_ptr->turysci_w_grupie=0;
	global_shm_ptr->ilosc_przewodnikow=0;
	global_shm_ptr->nr_przewodnika=1;
	
	// Most
	global_shm_ptr->liczba_osob_na_moscie=0;
	global_shm_ptr->most_kierunek=0;
	global_shm_ptr->czekajacy_przewodnicy_most=0;
	global_shm_ptr->przewodnicy_most=0;
	
	// Wieża
	global_shm_ptr->liczba_osob_na_wiezy=0;
	global_shm_ptr->pierwsza_klatka=0;
	global_shm_ptr->druga_klatka=0;
	for(int i=0; i<P; i++){
		global_shm_ptr->wieza_sygnal[i]=0;
	}
	
	// Prom
	global_shm_ptr->turysci_trasa_1=0;
	global_shm_ptr->turysci_trasa_2=0;
	global_shm_ptr->prom_kierunek=0;
	global_shm_ptr->prom_zajete=0;
	global_shm_ptr->prom_odplynal=0;
	
	// Wypisywanie informacji o symulacji tylko raz
	global_shm_ptr->prom_istnieje=0;
	global_shm_ptr->przewodnik_istnieje=0;
	global_shm_ptr->turysta_istnieje=0;
	global_shm_ptr->kasjer_istnieje=0;
	global_shm_ptr->kasjer_glowny=0;

	// Następne wywołanie kasjera zinicjalizuje wszystko na nowo
	global_shm_ptr->init=0;
	
    shmdt(global_shm_ptr);
	exit(1);
}
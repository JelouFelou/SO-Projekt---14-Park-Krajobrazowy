#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <math.h>
#include "header.h"

void przedwczesne_wyjscie(int);

int main() {
	// Inicjalizacja pamięci współdzielonej
	int shm_id;
    SharedData *shm_ptr = shm_get(&shm_id);
	
	struct komunikat kom;
	int id_przewodnik, id_turysta;
	int id_prom = getpid();
	int IDkolejki, semid_prom, semid_czekajaca_grupa;
	int wyczekuje=1;
	int i=0;
	
	if(shm_ptr->prom_odplynal==1){
		printf("Prom już odpłynął\n");
		exit(1);
	}else{
		shm_ptr->prom_odplynal=1;
	}
	
	printf(BLU "-------Symulacja parku krajobrazowego - Prom %d-------\n\n" RESET,id_prom);
	
	key_t key_kolejka = ftok(".", 99);
    if ((IDkolejki = msgget(key_kolejka, IPC_CREAT | 0600)) == -1) {
        perror("msgget() błąd");
        exit(1);
    }
	key_t key_prom = ftok(".", 202);
	if ((semid_prom = semget(key_prom, 1, IPC_CREAT | 0600)) == -1) {
		perror("Błąd przy tworzeniu semafora promu");
		exit(1);
	}
	key_t key_czekajaca_grupa = ftok(".", 204);
	if ((semid_czekajaca_grupa = semget(key_czekajaca_grupa, 1, IPC_CREAT | 0600)) == -1) {
		perror("Błąd przy tworzeniu semafora turysta_wchodzenie");
		exit(1);
	}
	
	// Inicjalizacja semaforów
	union semun arg;					
	arg.val = 0;
		semctl(semid_prom, 0, SETVAL, arg);
		semctl(semid_czekajaca_grupa, 0, SETVAL, arg);
	
	signal(SIGTERM,przedwczesne_wyjscie);
	
// ---- Pętla promu ----
	while(1) {
		if(wyczekuje==1){
			if(shm_ptr->prom_kierunek==0) printf(YEL "[Prom %d] wyczekuje turystów\n" RESET, id_prom);
			else printf(YEL"[%d][Prom %d] wyczekuje turystów\n" RESET,shm_ptr->prom_kierunek,id_prom);
			wyczekuje=0;
		}
		
// --- Wycieczka
		if (shm_ptr->prom_zajete == X3 || (shm_ptr->prom_kierunek == 1 && shm_ptr->turysci_trasa_1 == 0) || (shm_ptr->prom_kierunek == 2 && shm_ptr->turysci_trasa_2 == 0)){
	// Prom odpływa
			printf(BLU"[%d][Prom %d]: Odpłynął z strony %d\n"RESET,shm_ptr->prom_kierunek,id_prom,shm_ptr->prom_kierunek);
			sleep(TPROM);
			shm_ptr->prom_kierunek = (shm_ptr->prom_kierunek == 1) ? 2 : 1;
			printf(BLU"[%d][Prom %d]: Dopłynął na stronę %d\n"RESET,shm_ptr->prom_kierunek,id_prom,shm_ptr->prom_kierunek);
	
	// Prom dopływa na drugą stronę
		// Wysiadają przewodnicy
			for (int i = 0; i < X3; i++){
				if(grupa_przewodnicy[i]==0){
					break;
				}
				kom.mtype = PROM + id_przewodnik;
				sprintf(kom.mtext, "EXIT");
				msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
				shm_ptr->prom_zajete--;
			}
		// Wysiadają turyści	
			for (int i = 0; i < X3; i++){
				if(grupa_turysci[i]==0){
						break;
				}
				kom.mtype = PROM + id_turysta;
				sprintf(kom.mtext, "EXIT");
				msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
				shm_ptr->prom_zajete--;
			}
			printf(YEL"[%d][Prom %d] wszyscy pasażerowie opuścili prom\n"RESET,shm_ptr->prom_kierunek, id_prom);
			
		// Jeżeli jacyś turyści są po drugiej stronie
			if((shm_ptr->prom_kierunek==1 && shm_ptr->turysci_trasa_1==0 && shm_ptr->turysci_trasa_2>0) || (shm_ptr->prom_kierunek==2 && shm_ptr->turysci_trasa_2==0 && shm_ptr->turysci_trasa_1>0)){
				printf(YEL"[%d][Prom %d] Otrzymałem informację, że po drugiej stronie znajdują się turyści\n"RESET,shm_ptr->prom_kierunek, id_prom);
				printf(BLU"[%d][Prom %d]: Odpłynął z strony %d\n"RESET,shm_ptr->prom_kierunek,id_prom,shm_ptr->prom_kierunek);
				sleep(TPROM);
				shm_ptr->prom_kierunek = (shm_ptr->prom_kierunek == 1) ? 1 : 2;
				printf(BLU"[%d][Prom %d]: Dopłynął na stronę %d\n"RESET,shm_ptr->prom_kierunek,id_prom,shm_ptr->prom_kierunek);
			}
			
			printf("[%d][Prom %d]: Sygnalizuje turystom by wsiadać na prom\n",shm_ptr->prom_kierunek,id_prom);
			printf("\n");
			continue;
		}
		
// --- Wsiadanie turystów na prom
	// Pobranie turysty lub przewodnika z kolejki
        if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, PROM, 0) == -1) {
			perror("msgrcv failed");
		} 
	// Sprawdzamy, czy wiadomość zaczyna się od "[Turysta"
		else if (strncmp(kom.mtext, "[Turysta", 8) == 0) {
			if (sscanf(kom.mtext, "[Turysta %d] chce wejść na prom", &id_turysta) == 1) {
				if (!czy_istnieje(id_turysta)) continue;
				printf("[%d][Prom %d] Zaprasza turystę %d na prom\n", id_prom, id_turysta);
				kom.mtype = PROM + id_turysta;
				sprintf(kom.mtext, "OK");
				msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
				grupa_turysci[shm_ptr->prom_zajete]=id_turysta;
				shm_ptr->prom_zajete++;
			} 
			else {
				printf(RED"Błąd przy odczytywaniu wiadomości od turysty: %s\n"RESET, kom.mtext);
				continue;
			}
		}
    // Sprawdzamy, czy wiadomość zaczyna się od "[Przewodnik"
		else if (strncmp(kom.mtext, "[Przewodnik", 12) == 0) {
			if (sscanf(kom.mtext, "[Przewodnik %d] chce wejść na prom", &id_przewodnik) == 1) {
				if (!czy_istnieje(id_przewodnik)) continue;
				printf("[%d][Prom %d] Zaprasza przewodnika %d na prom\n", id_prom, id_przewodnik);
				kom.mtype = PROM + id_przewodnik;
				sprintf(kom.mtext, "OK");
				msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
				grupa_przewodnicy[shm_ptr->prom_zajete]=id_przewodnik;
				shm_ptr->prom_zajete++;
			} 
			else {
				printf(RED"Błąd przy odczytywaniu wiadomości od przewodnika: %s\n"RESET, kom.mtext);
				continue;
			}
		} 
	// Jeżeli otrzymana wiadomość nie zgadza się z żadnymi wytycznymi
		else {
			fprintf(stderr, "Nieznany format wiadomości: %s\n", kom.mtext);
			continue;
		}
	}
}

void przedwczesne_wyjscie(int sig_n){
	printf("\n[Prom] Odpływa żeby przejść prace konserwacyjne, ale pierwsze przewozi turystów na drugą stronę\n");
	int shm_id;
    SharedData *shm_ptr = shm_get(&shm_id);
	shm_ptr->prom_odplynal=0;
	
	// Wysiadają przewodnicy
	for (int i = 0; i < X3; i++){
		if(grupa_przewodnicy[i]==0){
			break;
		}
		kom.mtype = PROM + id_przewodnik;
		sprintf(kom.mtext, "EXIT");
		msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
		shm_ptr->prom_zajete--;
	}
	// Wysiadają turyści	
	for (int i = 0; i < X3; i++){
		if(grupa_turysci[i]==0){
				break;
		}
		kom.mtype = PROM + id_turysta;
		sprintf(kom.mtext, "EXIT");
		msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
		shm_ptr->prom_zajete--;
	}
	
	shmdt(shm_ptr);
    exit(1);
}
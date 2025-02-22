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

struct komunikat kom;
int id_przewodnik, id_turysta;
int IDkolejki, semid_prom, semid_czekajaca_grupa;
int grupa_przewodnicy[P], grupa_turysci[X3];
	

int main() {
	// Inicjalizacja pamięci współdzielonej
	int shm_id;
    SharedData *shm_ptr = shm_get(&shm_id);
	
	int id_prom = getpid();
	int wyczekuje=1;
	int to_przewodnik=0;
	int k=0;
	int l=0;
	
	if(shm_ptr->prom_istnieje==1){
		printf("Prom już odpłynął\n");
		exit(1);
	}else{
		shm_ptr->prom_istnieje=1;
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
	
	signal(SIGINT,przedwczesne_wyjscie); 
	signal(SIGTERM,przedwczesne_wyjscie);
	
// ---- Pętla promu ----
	while(1) {
		if(wyczekuje==1){
			if(shm_ptr->prom_kierunek==0) printf(YEL "[Prom %d] wyczekuje turystów\n" RESET, id_prom);
			else printf(YEL"[%d][Prom %d] wyczekuje turystów\n" RESET,shm_ptr->prom_kierunek,id_prom);
			wyczekuje=0;
		}
		
// --- Wycieczka
		if (shm_ptr->prom_zajete == X3 || 
		(shm_ptr->turysci_trasa_1 == 0 && shm_ptr->turysci_trasa_2 == 0 && shm_ptr->prom_zajete > 0) || 
		(shm_ptr->prom_kierunek==1 && shm_ptr->turysci_trasa_1 == 0 && shm_ptr->prom_zajete > 0) || 
		(shm_ptr->prom_kierunek==2 && shm_ptr->turysci_trasa_2 == 0 && shm_ptr->prom_zajete > 0)) {
	// Prom odpływa
			printf("Zajęte: %d; Kierunek: %d (1: %d | 2: %d) Prom odplynal: %d\n",shm_ptr->prom_zajete, shm_ptr->prom_kierunek, shm_ptr->turysci_trasa_1, shm_ptr->turysci_trasa_2, shm_ptr->prom_odplynal);
			shm_ptr->prom_odplynal=1;
			printf("Zajęte: %d; Kierunek: %d (1: %d | 2: %d) Prom odplynal: %d\n",shm_ptr->prom_zajete, shm_ptr->prom_kierunek, shm_ptr->turysci_trasa_1, shm_ptr->turysci_trasa_2, shm_ptr->prom_odplynal);
			printf(BLU"[%d][Prom %d]: Odpłynął z strony %d\n"RESET,shm_ptr->prom_kierunek,id_prom,shm_ptr->prom_kierunek);
			sleep(TPROM);
			shm_ptr->prom_kierunek = (shm_ptr->prom_kierunek == 1) ? 2 : 1;
			printf(BLU"[%d][Prom %d]: Dopłynął na stronę %d\n"RESET,shm_ptr->prom_kierunek,id_prom,shm_ptr->prom_kierunek);
	
	// Prom dopływa na drugą stronę		
			for (int i=0;i<X3;i++){
				if(grupa_turysci[i]==0){
					printf("Break turyści: %d\n",i);
					break;
				}
				to_przewodnik=0;
				shm_ptr->prom_zajete--;
				for(int j=0;j<P;j++){
					if(grupa_przewodnicy[j]==grupa_turysci[i]){
						printf("[%d][Prom %d] Przewodnik %d wysiada z promu (%d/%d)\n",shm_ptr->prom_kierunek, id_prom, id_przewodnik, shm_ptr->prom_zajete, X3);
						grupa_przewodnicy[j]=0;
						to_przewodnik=1;
						break;
					}
				}
				if(to_przewodnik==0){
					kom.mtype = PROM + grupa_turysci[i] + PROM_EXIT_OFFSET;
					sprintf(kom.mtext, "PROM");
					msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
					printf("[%d][Prom %d] Turysta %d wysiada z promu (%d/%d)\n",shm_ptr->prom_kierunek, id_prom, id_turysta, shm_ptr->prom_zajete, X3);
				}
				grupa_turysci[i]=0;
				sleep(1);
			}
			
			printf(YEL"[%d][Prom %d] wszyscy pasażerowie opuścili prom\n"RESET,shm_ptr->prom_kierunek, id_prom);
			
	// Jeżeli jeszcze jacyś turyści są po drugiej stronie
			if((shm_ptr->prom_kierunek==1 && shm_ptr->turysci_trasa_1==0 && shm_ptr->turysci_trasa_2>0) || (shm_ptr->prom_kierunek==2 && shm_ptr->turysci_trasa_2==0 && shm_ptr->turysci_trasa_1>0)){
				if(shm_ptr->prom_kierunek==1) printf(YEL"[%d][Prom %d] Otrzymałem informację, że po drugiej stronie znajdują się turyści: %d\n"RESET,shm_ptr->prom_kierunek, id_prom, shm_ptr->turysci_trasa_2);
				else printf(YEL"[%d][Prom %d] Otrzymałem informację, że po drugiej stronie znajdują się turyści: %d\n"RESET,shm_ptr->prom_kierunek, id_prom, shm_ptr->turysci_trasa_1);
				printf(BLU"[%d][Prom %d]: Odpłynął z strony %d\n"RESET,shm_ptr->prom_kierunek,id_prom,shm_ptr->prom_kierunek);
				sleep(TPROM);
				shm_ptr->prom_kierunek = (shm_ptr->prom_kierunek == 1) ? 1 : 2;
				printf(BLU"[%d][Prom %d]: Dopłynął na stronę %d\n"RESET,shm_ptr->prom_kierunek,id_prom,shm_ptr->prom_kierunek);
			}
			
			printf("[%d][Prom %d]: Sygnalizuje turystom by wsiadać na prom\n",shm_ptr->prom_kierunek,id_prom);
			shm_ptr->prom_odplynal=0;
			l=0;
			k=0;
			printf("%d\n",shm_ptr->prom_zajete);
			continue;
		}

		
// --- Wsiadanie turystów na prom
	//1. Wysyłamy komunikat do przewodnika o gotowości do przyjmowania nastęnej osoby
		kom.mtype = PROM + PROM_START_OFFSET;
		sprintf(kom.mtext, "OK");
		msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
		if(shm_ptr->prom_zajete == X3 || 
		(shm_ptr->turysci_trasa_1 == 0 && shm_ptr->turysci_trasa_2 == 0 && shm_ptr->prom_zajete > 0) || 
		(shm_ptr->prom_kierunek==1 && shm_ptr->turysci_trasa_1 == 0 && shm_ptr->prom_zajete > 0) || 
		(shm_ptr->prom_kierunek==2 && shm_ptr->turysci_trasa_2 == 0 && shm_ptr->prom_zajete > 0)) {
			continue;
		}
		printf("Wysyła komunikat OK.1\n");
		
	//2. Wyczekujemy na komunikat zwrotny
        if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, PROM + PROM_ENTER_OFFSET, 0) == -1) {
			perror("msgrcv failed");
			continue;
		}
		
	//2.1. Sprawdzamy, czy wiadomość zaczyna się od "[Przewodnik"
		if (strncmp(kom.mtext, "[Przewodnik", 11) == 0) {
			if (sscanf(kom.mtext, "[Przewodnik %d] chce wejść na prom", &id_przewodnik) == 1) {
				if (!czy_istnieje(id_przewodnik)) continue;
				shm_ptr->prom_zajete++;
				printf("[%d][Prom %d] Zaprasza przewodnika %d na prom (%d/%d)\n",shm_ptr->prom_kierunek, id_prom, id_przewodnik, shm_ptr->prom_zajete, X3);
			// Wysyłanie potwierdzenia do przewodnika
				kom.mtype = PROM + PROM_WELCOME_OFFSET;
				sprintf(kom.mtext, "WELCOME");
				msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
				printf("Wysyła komunikat OK.2\n");
				
				grupa_turysci[k]=id_przewodnik;
				grupa_przewodnicy[l]=id_przewodnik;
				k++;
				l++;
				printf("Przewodnik\n");
			} else {
				printf(RED"Błąd przy odczytywaniu wiadomości od przewodnika: %s\n"RESET, kom.mtext);
				continue;
			}
		} 
	//2.1. Sprawdzamy, czy wiadomość zaczyna się od "[Turysta"
		else if (strncmp(kom.mtext, "[Turysta", 8) == 0) {
			if (sscanf(kom.mtext, "[Turysta %d] chce wejść na prom", &id_turysta) == 1) {
				if (!czy_istnieje(id_turysta)) continue;
				shm_ptr->prom_zajete++;
				LiczbaTurysciTrasy(shm_ptr->prom_kierunek, shm_ptr); // Zmniejsza liczbę osób danej strony o 1
				printf("[%d][Prom %d] Zaprasza turystę %d na prom (%d/%d)\n",shm_ptr->prom_kierunek, id_prom, id_turysta, shm_ptr->prom_zajete, X3);
			// Wysyłanie potwierdzenia do przewodnika
				kom.mtype = PROM + PROM_WELCOME_OFFSET;
				sprintf(kom.mtext, "WELCOME");
				msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
				printf("Wysyła komunikat OK.3\n");
				grupa_turysci[k]=id_turysta;
				k++;
				printf("Turysta\n");
			} else {
				printf(RED"Błąd przy odczytywaniu wiadomości od turysty: %s\n"RESET, kom.mtext);
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
	int id_prom = getpid();
	
	// Wysiadają turyści
	for (int i=0;i<X3;i++){
		if(grupa_turysci[i]==0){
			printf("Break turyści: %d\n",i);
			break;
		}
		int to_przewodnik=0;
		kom.mtype = PROM + grupa_turysci[i] + PROM_EXIT_OFFSET;
		sprintf(kom.mtext, "PROM");
		msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
		shm_ptr->prom_zajete--;
		for(int j=0;j<P;j++){
			if(grupa_przewodnicy[j]==grupa_turysci[i]){
				printf("[%d][Prom %d] Przewodnik %d wysiada z promu (%d/%d)\n",shm_ptr->prom_kierunek, id_prom, id_przewodnik, shm_ptr->prom_zajete, X3);
				grupa_przewodnicy[j]=0;
				to_przewodnik=1;
				break;
			}
		}
		if(to_przewodnik==0){
			printf("[%d][Prom %d] Turysta %d wysiada z promu (%d/%d)\n",shm_ptr->prom_kierunek, id_prom, id_turysta, shm_ptr->prom_zajete, X3);
		}
		grupa_turysci[i]=0;
		sleep(1);
	}
	
	shm_ptr->prom_odplynal=0;
	shm_ptr->prom_istnieje=0;
	shm_ptr->prom_zajete=0;
	shm_ptr->prom_kierunek=0;
	
	shmdt(shm_ptr);
    exit(1);
}
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

SharedData *global_shm_ptr = 0;

void przedwczesne_wyjscie(int);

struct komunikat kom;
int id_przewodnik, id_turysta;
int IDkolejki, semid_prom, semid_czekajaca_grupa, semid_prom_check;
int grupa_przewodnicy[P], grupa_turysci[X3];
	
int to_przewodnik=0;

int main() {
	// Inicjalizacja pamięci współdzielonej
	int shm_id;
    SharedData *shm_ptr = shm_init(&shm_id);
	global_shm_ptr = shm_ptr;
	
	int id_prom = getpid();
	int wyczekuje=1;
	int k=0;
	int l=0;
	int wydluzenie=0;
	int wydluzenie_prom=0;
	int przewodnik_ok=0;
	shm_ptr->prom_zajete=0;
	
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
	key_t key_prom_check = ftok(".", 208);
	if ((semid_prom_check = semget(key_prom_check, 1, IPC_CREAT | 0600)) == -1) {
		perror("Błąd przy tworzeniu semafora most_wchodzenie");
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
		
		if(shm_ptr->prom_zajete==0 && ((shm_ptr->prom_kierunek==1 && shm_ptr->turysci_trasa_2 > 0 && shm_ptr->turysci_trasa_1 == 0) || (shm_ptr->prom_kierunek==2 && shm_ptr->turysci_trasa_1 > 0 && shm_ptr->turysci_trasa_2 == 0))){
			shm_ptr->prom_odplynal=1;
			printf(YEL"[%d][Prom %d] Otrzymałem informację, że po drugiej stronie znajdują się turyści2: %d\n"RESET,shm_ptr->prom_kierunek, id_prom, shm_ptr->turysci_trasa_1);
			printf(BLU"[%d][Prom %d] Odpłynął z strony %d\n"RESET,shm_ptr->prom_kierunek,id_prom,shm_ptr->prom_kierunek);
			sleep(5);
			shm_ptr->prom_kierunek = (shm_ptr->prom_kierunek == 1) ? 2 : 1;
			printf(BLU"[%d][Prom %d] Dopłynął na stronę %d\n"RESET,shm_ptr->prom_kierunek,id_prom,shm_ptr->prom_kierunek);
			shm_ptr->prom_odplynal=0;
		}
		
// --- Wycieczka
		if (shm_ptr->prom_zajete == X3 || 
		(shm_ptr->turysci_trasa_1 == 0 && shm_ptr->turysci_trasa_2 == 0 && shm_ptr->prom_zajete > 0) || 
		(shm_ptr->prom_kierunek==1 && shm_ptr->turysci_trasa_1 == 0 && shm_ptr->prom_zajete > 0) || 
		(shm_ptr->prom_kierunek==2 && shm_ptr->turysci_trasa_2 == 0 && shm_ptr->prom_zajete > 0)) {
	// Prom odpływa
			float czas = rand() % 10 + 1;
			if(wydluzenie_prom==1){
				czas *= 1.5;
			}
			
			shm_ptr->prom_odplynal=1;
			printf(BLU"[%d][Prom %d] Odpłynął z strony %d\n"RESET,shm_ptr->prom_kierunek,id_prom,shm_ptr->prom_kierunek);
			sleep(czas);
			shm_ptr->prom_kierunek = (shm_ptr->prom_kierunek == 1) ? 2 : 1;
			printf(BLU"[%d][Prom %d] Dopłynął na stronę %d\n"RESET,shm_ptr->prom_kierunek,id_prom,shm_ptr->prom_kierunek);
	
	// Prom dopływa na drugą stronę		
			for (int i=0;i<X3;i++){
				if(grupa_turysci[i]==0){
					break;
				}
				to_przewodnik=0;
				shm_ptr->prom_zajete--;
				for(int j=0;j<P;j++){
					if(grupa_przewodnicy[j]==grupa_turysci[i]){
						printf("[%d][Prom %d] Przewodnik %d wysiada z promu (%d/%d)\n",shm_ptr->prom_kierunek, id_prom, id_przewodnik, shm_ptr->prom_zajete, X3);
						/*kom.mtype = PROM + grupa_przewodnicy[j] + PROM_EXIT_OFFSET;
						sprintf(kom.mtext, "PROM");
						msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);*/
						grupa_przewodnicy[j]=0;
						to_przewodnik=1;
						break;
					}
				}
				if(to_przewodnik==0){
					kom.mtype = PROM + grupa_turysci[i] + PROM_EXIT_OFFSET;
					sprintf(kom.mtext, "PROM");
					msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
					printf("[%d][Prom %d] Turysta %d wysiada z promu (%d/%d)\n",shm_ptr->prom_kierunek, id_prom, grupa_turysci[i], shm_ptr->prom_zajete, X3);
				}
				
				grupa_turysci[i]=0;
				sleep(1);
			}
			
			printf(YEL"[%d][Prom %d] wszyscy pasażerowie opuścili prom\n"RESET,shm_ptr->prom_kierunek, id_prom);
			
	// Jeżeli jeszcze jacyś turyści są po drugiej stronie
			if((shm_ptr->prom_kierunek==1 && shm_ptr->turysci_trasa_1==0 && shm_ptr->turysci_trasa_2>0) || (shm_ptr->prom_kierunek==2 && shm_ptr->turysci_trasa_2==0 && shm_ptr->turysci_trasa_1>0)){
				if(shm_ptr->prom_kierunek==1) printf(YEL"[%d][Prom %d] Otrzymałem informację, że po drugiej stronie znajdują się turyści: %d\n"RESET,shm_ptr->prom_kierunek, id_prom, shm_ptr->turysci_trasa_2);
				else printf(YEL"[%d][Prom %d] Otrzymałem informację, że po drugiej stronie znajdują się turyści: %d\n"RESET,shm_ptr->prom_kierunek, id_prom, shm_ptr->turysci_trasa_1);
				printf(BLU"[%d][Prom %d] Odpłynął z strony %d\n"RESET,shm_ptr->prom_kierunek,id_prom,shm_ptr->prom_kierunek);
				sleep(czas);
				shm_ptr->prom_kierunek = (shm_ptr->prom_kierunek == 1) ? 2 : 1;
				printf(BLU"[%d][Prom %d] Dopłynął na stronę %d\n"RESET,shm_ptr->prom_kierunek,id_prom,shm_ptr->prom_kierunek);
			}
			
			printf("[%d][Prom %d] Sygnalizuje turystom by wsiadać na prom\n",shm_ptr->prom_kierunek,id_prom);
			shm_ptr->prom_odplynal=0;
			l=0;
			k=0;
			//printf("prom_zajete: %d\n",shm_ptr->prom_zajete);
			wydluzenie_prom=0;
			continue;
		}

		
// --- Wsiadanie turystów na prom
	//1. Wysyłamy komunikat do przewodnika o gotowości do przyjmowania nastęnej osoby
		if(przewodnik_ok==0){
			kom.mtype = PROM + PROM_START_OFFSET;
			sprintf(kom.mtext, "OK");
			msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
			przewodnik_ok=1;
		}
		
		if(shm_ptr->prom_zajete == X3 || 
		(shm_ptr->turysci_trasa_1 == 0 && shm_ptr->turysci_trasa_2 == 0 && shm_ptr->prom_zajete > 0) || 
		(shm_ptr->prom_kierunek==1 && shm_ptr->turysci_trasa_1 == 0 && shm_ptr->prom_zajete > 0) || 
		(shm_ptr->prom_kierunek==2 && shm_ptr->turysci_trasa_2 == 0 && shm_ptr->prom_zajete > 0)) {
			continue;
		}
		//printf("Wysyła komunikat OK.1\n");
		
	//2. Wyczekujemy na komunikat zwrotny
        if (msgrcv(IDkolejki, (struct msgbuf *)&kom, MAX, PROM + PROM_ENTER_OFFSET, IPC_NOWAIT) == -1) {
			if (errno == ENOMSG) {
				continue;
			} else {
				perror("msgrcv failed");
				continue;
			}	
		}else{
			przewodnik_ok=0;
		//2.1. Sprawdzamy, czy wiadomość zaczyna się od "[Przewodnik"
			if (strncmp(kom.mtext, "[Przewodnik", 11) == 0) {
			if (sscanf(kom.mtext, "[Przewodnik %d] chce wejść na prom", &id_przewodnik) == 1) {
				if (!czy_istnieje(id_przewodnik)) continue;
				shm_ptr->prom_zajete++;
				printf("[%d][Prom %d] Zaprasza przewodnika %d na prom (%d/%d) (1:%d 2:%d)\n",shm_ptr->prom_kierunek, id_prom, id_przewodnik, shm_ptr->prom_zajete, X3, shm_ptr->turysci_trasa_1, shm_ptr->turysci_trasa_2);
			// Wysyłanie potwierdzenia do przewodnika
				kom.mtype = PROM + PROM_WELCOME_OFFSET;
				sprintf(kom.mtext, "WELCOME");
				msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
				//printf("Wysyła komunikat OK.2\n");
				
				grupa_turysci[k]=id_przewodnik;
				grupa_przewodnicy[l]=id_przewodnik;
				k++;
				l++;
				//printf("Przewodnik\n");
			} else {
				printf(RED"Błąd przy odczytywaniu wiadomości od przewodnika: %s\n"RESET, kom.mtext);
				continue;
			}
		} 
		//2.1. Sprawdzamy, czy wiadomość zaczyna się od "[Turysta"
			else if (strncmp(kom.mtext, "[Turysta", 8) == 0) {
			if (sscanf(kom.mtext, "[Turysta %d] chce wejść na prom %d", &id_turysta, &wydluzenie) == 2) {
				if (!czy_istnieje(id_turysta)) continue;
				if (wydluzenie) wydluzenie_prom=1;
				
				shm_ptr->prom_zajete++;
				semafor_operacja(semid_prom_check, -1);
				LiczbaTurysciTrasy(shm_ptr->prom_kierunek, shm_ptr); // Zmniejsza liczbę osób danej strony o 1
				semafor_operacja(semid_prom_check, 1);
				printf("[%d][Prom %d] Zaprasza turystę %d na prom (%d/%d) (1:%d 2:%d)\n",shm_ptr->prom_kierunek, id_prom, id_turysta, shm_ptr->prom_zajete, X3, shm_ptr->turysci_trasa_1, shm_ptr->turysci_trasa_2);
			// Wysyłanie potwierdzenia do przewodnika
				kom.mtype = PROM + PROM_WELCOME_OFFSET;
				sprintf(kom.mtext, "WELCOME");
				msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
				//printf("Wysyła komunikat OK.3\n");
				grupa_turysci[k]=id_turysta;
				k++;
				//printf("Turysta\n");
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
}

void przedwczesne_wyjscie(int sig_n){
	int id_prom = getpid();
	
	if(global_shm_ptr->kasjer_glowny==2){
		printf("\n[Prom %d] opuszcza park po całym dniu ciężkiej pracy\n", id_prom);
	}else{
		printf("\n[Prom %d] Odpływa żeby przejść prace konserwacyjne, ale pierwsze przewozi turystów na drugą stronę\n",id_prom);
	}
	
	
	global_shm_ptr->prom_istnieje=0;
	// Wysiadają turyści
	for (int i=0;i<X3;i++){
		if(grupa_turysci[i]==0){
			//printf("Break turyści: %d\n",i);
			break;
		}
		to_przewodnik=0;
		global_shm_ptr->prom_zajete--;
		for(int j=0;j<P;j++){
			if(grupa_przewodnicy[j]==grupa_turysci[i]){
				printf("[%d][Prom %d] Przewodnik %d wysiada z promu (%d/%d)\n",global_shm_ptr->prom_kierunek, id_prom, id_przewodnik, global_shm_ptr->prom_zajete, X3);
				kill(grupa_przewodnicy[j], SIGUSR2);
				grupa_przewodnicy[j]=0;
				to_przewodnik=1;
				break;
			}
		}
		if(to_przewodnik==0){
			kom.mtype = PROM + grupa_turysci[i] + PROM_EXIT_OFFSET;
			sprintf(kom.mtext, "PROM");
			msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
			printf("[%d][Prom %d] Turysta %d wysiada z promu (%d/%d)\n",global_shm_ptr->prom_kierunek, id_prom, id_turysta, global_shm_ptr->prom_zajete, X3);
		}
		grupa_turysci[i]=0;
	}
	
	printf("\n[Prom %d] Wszyscy turyści opuścili prom\n",id_prom);
	
	shmdt(global_shm_ptr);
    exit(1);
}
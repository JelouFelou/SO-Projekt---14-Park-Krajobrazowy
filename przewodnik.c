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
#include "trasa.h"

void awaryjne_wyjscie(int);
void rozpoczecie_wycieczki(int);
void przedwczesne_wyjscie(int);
void wyjscie_prom(int);

int grupa[M];
int wiek_turysty[M];
int liczba_w_grupie=0;
int IDkolejki;
int semid_most, semid_wieza, semid_prom, semid_turysta_wchodzenie, semid_czekajaca_grupa, semid_most_wchodzenie, semid_przewodnik, semid_prom_check;
int wymuszony_start=0;
int przypisana_trasa=0;


int main() {
	// Inicjalizacja pamięci współdzielonej
	int shm_id;
    SharedData *shm_ptr = shm_init(&shm_id);
	global_shm_ptr = shm_ptr;
	
	if(shm_ptr->ilosc_przewodnikow==P){
		printf("W danej chwili może być uruchomionych max %d przewodników\n",P);
		exit(1);
	}else{
		shm_ptr->ilosc_przewodnikow++;
	}
	
	if(X1>=M){
		printf("X1 musi być mniejsze od M, aktualnie M = %d\n",M);
		return 0;
	}
	if(X2>=(2*M)){
		printf("X2 musi być mniejsze od 2M, aktualnie 2M = %d\n",2*M);
		return 0;
	}
	if(X3>=(1.5*M)){
		printf("X3 musi być mniejsze od 1.5M, aktualnie 1.5M = %.2f\n",1.5*M);
		return 0;
	}
	
	struct komunikat kom;
	int id_przewodnik = getpid();
	int id_turysta, typ_trasy = 0, wiek, id_kasjer;
	int wyczekuje = 1;
	int pomylka = 0;
	int odbiera = 1;
	int wydluzenie=0;
	float czas_trasa=0;
	int opiekun=0;
	
	
	if(shm_ptr->przewodnik_istnieje==0){
		shm_ptr->przewodnik_istnieje=1;
		printf(GRN "-------Symulacja parku krajobrazowego - Przewodnik-------\n\n" RESET);
	}
	
	// Pobieranie ID kolejki komunikatów
	key_t key_kolejka = ftok(".", 99);
    if ((IDkolejki = msgget(key_kolejka, IPC_CREAT | 0600)) == -1) {
        perror("msgget() błąd");
        exit(1);
    }
	key_t key_przewodnik = ftok(".", 100);
    if ((semid_przewodnik = semget(key_przewodnik, 1, IPC_CREAT | 0600)) == -1) {
		perror("Błąd przy tworzeniu semafora mostu");
		exit(1);
	}
	
	// Odpowiedzialne za kontrolę dostępu do atrakcji przez innych przewodników
	key_t key_most = ftok(".", 200);
	if ((semid_most = semget(key_most, 1, IPC_CREAT | 0600)) == -1) {
		perror("Błąd przy tworzeniu semafora mostu");
		exit(1);
	}
	key_t key_wieza = ftok(".", 201);
	if ((semid_wieza = semget(key_wieza, 1, IPC_CREAT | 0600)) == -1) {
		perror("Błąd przy tworzeniu semafora wieży");
		exit(1);
	}
	key_t key_prom = ftok(".", 202);
	if ((semid_prom = semget(key_prom, 1, IPC_CREAT | 0600)) == -1) {
		perror("Błąd przy tworzeniu semafora promu");
		exit(1);
	}
	key_t key_turysta_wchodzenie = ftok(".", 203);
	if ((semid_turysta_wchodzenie = semget(key_turysta_wchodzenie, 1, IPC_CREAT | 0600)) == -1) {
		perror("Błąd przy tworzeniu semafora turysta_wchodzenie");
		exit(1);
	}
	key_t key_czekajaca_grupa = ftok(".", 204);
	if ((semid_czekajaca_grupa = semget(key_czekajaca_grupa, 1, IPC_CREAT | 0600)) == -1) {
		perror("Błąd przy tworzeniu semafora turysta_wchodzenie");
		exit(1);
	}
	key_t key_most_wchodzenie = ftok(".", 205);
	if ((semid_most_wchodzenie = semget(key_most_wchodzenie, 1, IPC_CREAT | 0600)) == -1) {
		perror("Błąd przy tworzeniu semafora most_wchodzenie");
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
		semctl(semid_most, 0, SETVAL, arg);
		semctl(semid_wieza, 0, SETVAL, arg);
		semctl(semid_prom, 0, SETVAL, arg);
		semctl(semid_czekajaca_grupa, 0, SETVAL, arg);
	arg.val = 1;
		semctl(semid_turysta_wchodzenie, 0, SETVAL, arg);
		semctl(semid_most_wchodzenie, 0, SETVAL, arg);
		semctl(semid_przewodnik, 0, SETVAL, arg);
		semctl(semid_prom_check, 0, SETVAL, arg);
		
	
	// Po nacisnieciu przez uzytkownika CTRL+C wywoluje sie funkcja awaryjne_wyjscie()
	signal(SIGINT,awaryjne_wyjscie); 
	signal(SIGUSR1,rozpoczecie_wycieczki);
	signal(SIGTERM,przedwczesne_wyjscie);
	signal(SIGUSR2,wyjscie_prom);

	semafor_operacja(semid_przewodnik, -1);
	int numer=shm_ptr->nr_przewodnika;
	shm_ptr->nr_przewodnika++;
	semafor_operacja(semid_przewodnik, 1);

// ---- Pętla przewodnika ----
    while (1) {
		if(wyczekuje){ // Wyświetli tylko raz | wyczekuje turystę zamiast za każdym razem jak turysta dołączy
			printf(YEL "[Przewodnik %d]: wyczekuje turystów\n" RESET, id_przewodnik);
			wyczekuje=0;
		}
		
// --- Wycieczka
		if (liczba_w_grupie == M || wymuszony_start == 1 || (shm_ptr->liczba_turystow==shm_ptr->turysci_w_grupie && shm_ptr->turysci_w_grupie!=0 && przypisana_trasa!=0)) {
			sleep(2);
			wymuszony_start=0;
            printf(GRN"\n[%d][Przewodnik %d]: \"Grupa zapełniona (%d osób)! Oprowadzę was po trasie %d\"\n"RESET,przypisana_trasa, id_przewodnik, liczba_w_grupie, typ_trasy);
            sleep(1);
			
			for (int i = 0; i < liczba_w_grupie; i++) {
				if(wiek_turysty[i]<18){
					opiekun=1;
					break;
				}
			}

			for (int i = 0; i < liczba_w_grupie; i++) {
				if(opiekun==1 && wiek_turysty[i]>18){ // w grupie jest tylko 1 opiekun dla wszystkich nieletnich
					opiekun=grupa[i];
					kom.mtype = grupa[i];
					sprintf(kom.mtext, "OK %d",1);
					msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
				}else{
					kom.mtype = grupa[i];
					sprintf(kom.mtext, "OK %d",0);
					msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
				}
			}
			for (int j = 0; j < liczba_w_grupie; j++){
				if(wiek_turysty[j]<12){
					wydluzenie=1;
					printf("[%d][Przewodnik %d]: W naszej grupie jest osoba poniżej 12 roku życia, wycieczka zajmie dłużej\n",przypisana_trasa, id_przewodnik);
					break;
				}
			}
			
			czas_trasa = rand() % 2 + 1;
			if(wydluzenie==1){
				czas_trasa *= 1.5;
			}
			
			// Start wycieczki zależny od przypisana_trasa
			switch(przypisana_trasa) {
			case 1:
				printf("[%d][Przewodnik %d]: Jesteśmy przy kasach\n",przypisana_trasa, id_przewodnik);
				sleep(czas_trasa);
				TrasaA(IDkolejki, przypisana_trasa, semid_most, semid_most_wchodzenie, id_przewodnik, grupa, liczba_w_grupie, wydluzenie, shm_ptr);
				sleep(czas_trasa);
				TrasaB(IDkolejki, przypisana_trasa, semid_wieza, id_przewodnik, grupa, wiek_turysty, liczba_w_grupie, wydluzenie, numer, shm_ptr);
				sleep(czas_trasa);
				TrasaC(IDkolejki, przypisana_trasa, semid_prom_check, semid_prom, semid_turysta_wchodzenie, semid_czekajaca_grupa, id_przewodnik, grupa, liczba_w_grupie, wiek_turysty, shm_ptr);
				sleep(czas_trasa);
				printf("[%d][Przewodnik %d]: Wróciliśmy do kas\n",przypisana_trasa, id_przewodnik);
				break;
			case 2:
				printf("[%d][Przewodnik %d]: Jesteśmy przy kasach\n",przypisana_trasa, id_przewodnik);
				sleep(czas_trasa);
				TrasaC(IDkolejki, przypisana_trasa, semid_prom_check, semid_prom, semid_turysta_wchodzenie, semid_czekajaca_grupa, id_przewodnik, grupa, liczba_w_grupie, wiek_turysty, shm_ptr);
				sleep(czas_trasa);
				TrasaB(IDkolejki, przypisana_trasa, semid_wieza, id_przewodnik, grupa, wiek_turysty, liczba_w_grupie, wydluzenie, numer, shm_ptr);
				sleep(czas_trasa);
				TrasaA(IDkolejki, przypisana_trasa, semid_most, semid_most_wchodzenie, id_przewodnik, grupa, liczba_w_grupie, wydluzenie, shm_ptr);
				sleep(czas_trasa);
				printf("[%d][Przewodnik %d]: Wróciliśmy do kas\n",przypisana_trasa, id_przewodnik);
				break;
			default:
				printf(YEL"\n[Przewodnik %d]: Coś mi tu nie gra... nie mam kogo oprowadzać...\n"RESET, id_przewodnik);
				pomylka=1;
			}
			
			if(!pomylka){
				//X. Zerowanie grupy przed każdorazowym użyciem
				for (int i = 0; i < liczba_w_grupie; i++) {
					kom.mtype = grupa[i]; 
					sprintf(kom.mtext, "END");
					msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
				}
				for (int i = 0; i < M; i++) {
					grupa[i] = 0; // Upewnij się, że grupa jest początkowo pusta
				}
				printf("[Przewodnik %d]: Wszyscy turyści bezpiecznie dotarli do kasy.\n",id_przewodnik);
			} else { // W wypadku gdy było zero turystów, wykonuje się ta część
				sleep(1);
				pomylka=0;
			}
			liczba_w_grupie = 0;
			przypisana_trasa = 0;
			wyczekuje = 1;
			wydluzenie = 0;
			czas_trasa = 0;
			opiekun = 0;
			printf("\n");
        }
		
// --- Przyjmowanie turysty do grupy		
		//5. Przyjmowanie turystów
        if (msgrcv(IDkolejki, &kom, MAX, PRZEWODNIK, IPC_NOWAIT) == -1) {
			if (errno==ENOMSG && shm_ptr->liczba_turystow==shm_ptr->turysci_w_grupie) {
				//printf(YEL"\n[Przewodnik %d]: To już ostatni turyści w parku\n"RESET, id_przewodnik);
				continue;
			}else if(errno==ENOMSG){
				// Brak wiadomości
				continue;
			}else if (wymuszony_start){
				printf(YEL"\n[Przewodnik %d]: Dostałem sygnał wymuszonego startu wycieczki\n"RESET, id_przewodnik);
			}else{
				perror("msgrcv failed");
				continue;
			} 
		} else {
			// Odczytujemy informacje o turyście
			sscanf(kom.mtext, "%d %d %d %d", &id_turysta, &typ_trasy, &wiek, &id_kasjer);
			if(odbiera){
				//printf("[Przewodnik %d]: Odbiera informacje od kasjera o turyście (%d %d %d %d)\n", id_przewodnik, id_turysta, typ_trasy, wiek, id_kasjer);
				odbiera=0;
			}
			
			// Sprawdzamy czy turysta o tym PID nadal istnieje
			if (!czy_istnieje(id_kasjer)) continue;
			if (!czy_istnieje(id_turysta)) continue;
		
			//6. Sprawdzamy przypisaną trasę
			if (liczba_w_grupie == 0) {
				// Pierwszy turysta – przypisujemy przewodnikowi dany typ
				przypisana_trasa = typ_trasy;
				printf("[Przewodnik %d]: Ustawiam typ trasy na %d\n", id_przewodnik, przypisana_trasa);
			}
			
			// Kolejne zgłoszenia – sprawdzamy zgodność typu trasy
			if (typ_trasy != przypisana_trasa) {
				//printf(RED "[%d][Przewodnik %d]: Odrzucam turystę %d, typ trasy %d nie pasuje do mojej trasy %d\n" RESET,przypisana_trasa,id_przewodnik, id_turysta, typ_trasy, przypisana_trasa);
				kom.mtype = id_kasjer;
				strcpy(kom.mtext, "NO");
				msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
				continue;
			} else if (typ_trasy == przypisana_trasa) {
				kom.mtype = id_kasjer;
				sprintf(kom.mtext, "OK %d %d", id_przewodnik, numer);
				msgsnd(IDkolejki, &kom, strlen(kom.mtext) + 1, 0);
				
				odbiera = 1;
				grupa[liczba_w_grupie] = id_turysta;
				wiek_turysty[liczba_w_grupie] = wiek;
				liczba_w_grupie++;
				printf(BLU"[%d][Przewodnik %d]>>>[Turysta %d](wiek %d) dołącza do trasy %d (%d/%d)\n"RESET,przypisana_trasa, id_przewodnik, id_turysta, wiek, typ_trasy, liczba_w_grupie, M);
				shm_ptr->turysci_w_grupie++;
				//printf("%d/%d\n",shm_ptr->turysci_w_grupie,shm_ptr->liczba_turystow);
			}
		}
    }
	return 0;
}

void awaryjne_wyjscie(int sig_n) {
    extern int grupa[M];  // Przypisanie grupy turystów z głównego programu
    extern int liczba_w_grupie;  // Liczba turystów w grupie
    extern int IDkolejki;  // ID kolejki komunikatów
    extern int semid_most, semid_wieza, semid_prom;
	extern int semid_wycieczka, semid_wyjscie, semid_przeplyniecie;
    extern int semid_turysta_most, semid_turysta_wieza, semid_turysta_prom;
	extern int semid_przewodnik_most, semid_przewodnik_prom, semid_przewodnik_wieza;
    
	global_shm_ptr->ilosc_przewodnikow--;

    struct komunikat kom;

    printf(RED"\nWyjście awaryjne! Turystyka przerywana! Prosimy o powrót do kasy!\n"RESET);

	// Kończy procesy turystów z jego grupy
	for (int i = 0; i < liczba_w_grupie; i++){
		kill(grupa[i], SIGTERM);
	}
	
	// Zwolnienie miejsc w semaforach dla turystów, aby uniknąć blokady
    for (int i = 0; i < liczba_w_grupie; i++) {
        semafor_operacja(semid_most, 1); // Zwolnienie mostu
        semafor_operacja(semid_wieza, 1); // Zwolnienie wieży
        semafor_operacja(semid_prom, 1); // Zwolnienie promu
    }

    // Przywracamy stan początkowy grupy
    liczba_w_grupie = 0;  // Grupa została opróżniona
	for (int i = 0; i < M; i++) {
        grupa[i] = 0;
    }

    // Zamykamy proces po wykonaniu awaryjnego wyjścia
    exit(0);
}

void rozpoczecie_wycieczki(int sig_n){
	wymuszony_start=1;
}

void przedwczesne_wyjscie(int sig_n){
	int id_przewodnik = getpid();
	if(global_shm_ptr->kasjer_glowny==2){
		printf("[Przewodnik %d] opuszcza park po całym dniu ciężkiej pracy\n", id_przewodnik);
	}else{
		printf("[Przewodnik %d] przedwcześnie opuszcza park\n", id_przewodnik);
	}
	
	global_shm_ptr->ilosc_przewodnikow--;
	for (int i = 0; i < liczba_w_grupie; i++){
		kill(grupa[i], SIGTERM);
	}
	shmdt(global_shm_ptr);
    exit(1);
}

void wyjscie_prom(int sig_n){
	int id_przewodnik = getpid();
	printf("[Przewodnik %d]: Przeplynąłem promem na drugą stronę\n", id_przewodnik);
}
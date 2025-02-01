#ifndef TRASA_H
#define TRASA_H

#include "header.h"

#define X1 2   // Maksymalna liczba osób na moście
#define X2 2   // Maksymalna liczba osób na wieży
#define X3 2   // Maksymalna liczba osób na promie

extern int *licznik_most;

// -------Most Wiszący-------
void TrasaA(int IDkolejki, int semid_most, int id_przewodnik, int grupa[], int liczba_w_grupie) {
    printf(GRN "\n-------Most Wiszący-------\n\n" RESET);
    printf("[Przewodnik %d]: Sprawdzam most wiszący...\n", id_przewodnik);

    if (*licznik_most == 0) {
        printf("[Przewodnik %d]: Na moście nie ma aktualnie nikogo!\n", id_przewodnik);
    }
	
	semafor_operacja(semid_most, -1);
	(*licznik_most)++;
	printf("[Przewodnik %d]: Wchodzę na most jako pierwszy z naszej grupy\n", id_przewodnik);
	
	
    // Wchodzenie turystów na most
    for (int i = 0; i < liczba_w_grupie; i++) {
		// Zajęcie miejsca na moście
		semafor_operacja(semid_most, -1);
		(*licznik_most)++;
		if(i==0){
			printf("[Turysta %d]: Wchodzę na most...\n", grupa[i]); // to powinno znaleźć się w turysta.c
			sleep(rand() % 1 + 1);
			
			// Przewodnik schodzi z mostu
			semafor_operacja(semid_most, 1);
			(*licznik_most)--;
			printf("[Przewodnik %d]: Przeszedłem przez most i czekam na moją grupę\n", id_przewodnik);
			sleep(1);
		}
		
		// Zwolnienie miejsca na moście
		printf("[Turysta %d]: Dotarłem na koniec mostu...\n", grupa[i]);
        semafor_operacja(semid_most, 1);
        (*licznik_most)--;

        printf("[Turysta %d]: Zszedłem z mostu.\n", grupa[i]);
    }
	
    printf("[Przewodnik %d]: Wszyscy z mojej grupy przeszli przez most.\n", id_przewodnik);
	printf("[Przewodnik %d]: Możemy w takim wypadku iść dalej.\n", id_przewodnik);
}


// -------Wieża Widokowa-------
void TrasaB(int IDkolejki, int semid_wieza, int id_przewodnik, int grupa[], int liczba_w_grupie) {
    printf(GRN "\n-------Wieża Widokowa-------\n\n" RESET);
	printf("[Przewodnik %d]: Grupa idzie na wieżę widokową.\n", id_przewodnik);



    sleep(rand() % 5 + 3); // Czas spędzony na wieży



    printf("[Przewodnik %d]: Wszyscy zeszli z wieży.\n", id_przewodnik);
}


// -------Płynięcie promem-------
void TrasaC(int IDkolejki, int semid_prom, int id_przewodnik, int grupa[], int liczba_w_grupie) {
    printf(GRN "\n-------Płynięcie promem-------\n\n" RESET);
	printf("[Przewodnik %d]: Grupa czeka na prom.\n", id_przewodnik);


    printf("[Przewodnik %d]: Prom odpływa...\n", id_przewodnik);
    sleep(rand() % 5 + 3); // Czas przepłynięcia

    printf("[Przewodnik %d]: Prom dopłynął, grupa wysiada.\n", id_przewodnik);
}

#endif
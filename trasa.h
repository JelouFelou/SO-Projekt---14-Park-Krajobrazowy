#ifndef TRASA_H
#define TRASA_H

#include "header.h"

#define X1 2   // Maksymalna liczba osób na moście (X1<M)
#define X2 3   // Maksymalna liczba osób na wieży (X2<2M)
#define X3 3   // Maksymalna liczba osób na promie (X3<1.5*M)

// -------Most Wiszący-------
void TrasaA(int IDkolejki, int semid_most, int semid_turysta_most, int id_przewodnik, int grupa[], int liczba_w_grupie) {
    printf(GRN "\n-------Most Wiszący-------\n\n" RESET);
    printf("[Przewodnik %d]: Sprawdzam most wiszący...\n", id_przewodnik);
	
	semafor_operacja(semid_most, -1);
	printf("[Przewodnik %d]: Wchodzę na most jako pierwszy z naszej grupy\n", id_przewodnik);
	printf("[Przewodnik %d]: Przeszedłem przez most i czekam na moją grupę\n", id_przewodnik);
	sleep(1);
	
    // Wchodzenie turystów na most
    for (int i = 0; i < liczba_w_grupie; i++) {
		semafor_operacja(semid_turysta_most, 1);
		sleep(1);
    }
	semafor_operacja(semid_most, 1);
    printf("[Przewodnik %d]: Wszyscy z mojej grupy przeszli przez most.\n", id_przewodnik);
	printf("[Przewodnik %d]: Możemy w takim wypadku iść dalej.\n", id_przewodnik);
}


// -------Wieża Widokowa-------
void TrasaB(int IDkolejki, int semid_wieza, int id_przewodnik, int grupa[], int liczba_w_grupie) {
    printf(GRN "\n-------Wieża Widokowa-------\n\n" RESET);
	printf("[Przewodnik %d]: Wejdźcie na wieże, ja będę czekać na dole\n", id_przewodnik);


	// Wchodzenie turystów na wieżę
    for (int i = 0; i < liczba_w_grupie; i++) {
        semafor_operacja(semid_wieza, -1); // Zajęcie miejsca na wieży
        printf("[Turysta %d]: Wchodzę na wieżę...\n", grupa[i]);
        sleep(rand() % 2 + 1);
    }
    
    sleep(rand() % 5 + 3); // Czas spędzony na wieży

    // Schodzenie turystów z wieży
    for (int i = 0; i < liczba_w_grupie; i++) {
        printf("[Turysta %d]: Schodzę z wieży...\n", grupa[i]);
        sleep(rand() % 2 + 1);
        semafor_operacja(semid_wieza, 1); // Zwolnienie miejsca na wieży
    }

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
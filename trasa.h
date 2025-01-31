#ifndef TRASA_H
#define TRASA_H

#define X1 2   // Maksymalna liczba osób na moście
#define X2 2   // Maksymalna liczba osób na wieży
#define X3 2   // Maksymalna liczba osób na promie



void TrasaA(int IDkolejki, int semid_most, int semid_kierunek, int id_przewodnik, int grupa[], int liczba_w_grupie) {
    printf(GRN "\n-------Most Wiszący-------\n\n" RESET);
    printf("[Przewodnik %d]: Sprawdzam most wiszący...\n", id_przewodnik);
    
    // Sprawdzanie, czy most jest dostępny w odpowiednim kierunku (semid_kierunek)
    struct sembuf kierunek = {0, -1, 0}; // Oczekuje na dostępność kierunku mostu (zablokowanie kierunku)
    if (semop(semid_kierunek, &kierunek, 1) == -1) {
        perror("Błąd semop przy sprawdzaniu kierunku");
        return;
    }
    // Czekanie na możliwość wejścia na most (semafor kontrolujący wejścia)
    struct sembuf wejscie = {0, -1, 0}; // Oczekuje na wolne miejsce na moście
    if (semop(semid_most, &wejscie, 1) == -1) {
        perror("Błąd semop przy czekaniu na most");
        return;
    }
    
	// --------Wejście na most--------
    printf("[Przewodnik %d]: Wchodzę na most jako pierwszy.\n", id_przewodnik);
    sleep(1);
    // Iterowanie po turystach w grupie i wypisywanie ich ID po przejściu przez most
    for (int i = 0; i < liczba_w_grupie; i++) {
        printf("[Turysta %d]: Przechodzę przez most...\n", grupa[i]);
        sleep(rand() % 2 + 1); // Symulacja czasu przejścia turysty
    }
    printf("[Przewodnik %d]: Wszyscy przeszli most.\n", id_przewodnik);
	
	
    // Zwolnienie semafora dla kierunku (umożliwienie innym grupom przejścia)
    struct sembuf wyjscie_kierunek = {0, 1, 0};  // Zwalnianie semafora kierunku
    if (semop(semid_kierunek, &wyjscie_kierunek, 1) == -1) {
        perror("Błąd semop przy zwalnianiu semafora kierunku");
        return;
    }
    // Zwolnienie miejsca na moście
    struct sembuf wyjscie = {0, 1, 0};
    if (semop(semid_most, &wyjscie, 1) == -1) {
        perror("Błąd semop przy zwalnianiu semafora mostu");
        return;
    }
    // Zmiana kierunku mostu, aby inna grupa mogła przejść
    struct sembuf zmiana_kierunku = {0, 1, 0}; // Zmiana kierunku mostu
    if (semop(semid_kierunek, &zmiana_kierunku, 1) == -1) {
        perror("Błąd semop przy zmianie kierunku mostu");
        return;
    }
}



void TrasaB(int IDkolejki, int semid_wieza, int id_przewodnik, int grupa[], int liczba_w_grupie) {
    printf(GRN "\n-------Wieża Widokowa-------\n\n" RESET);
	printf("[Przewodnik %d]: Grupa idzie na wieżę widokową.\n", id_przewodnik);

    // Turyści wchodzą na wieżę
    for (int i = 0; i < liczba_w_grupie; i++) {
        struct sembuf wejscie = {0, -1, 0};
        semop(semid_wieza, &wejscie, 1);
    }

    sleep(rand() % 5 + 3); // Czas spędzony na wieży

    // Turyści schodzą z wieży
    for (int i = 0; i < liczba_w_grupie; i++) {
        struct sembuf wyjscie = {0, 1, 0};
        semop(semid_wieza, &wyjscie, 1);
    }

    printf("[Przewodnik %d]: Wszyscy zeszli z wieży.\n", id_przewodnik);
}



void TrasaC(int IDkolejki, int semid_prom, int id_przewodnik, int grupa[], int liczba_w_grupie) {
    printf(GRN "\n-------Płynięcie promem-------\n\n" RESET);
	printf("[Przewodnik %d]: Grupa czeka na prom.\n", id_przewodnik);

    // Wejście na prom
    for (int i = 0; i < liczba_w_grupie; i++) {
        struct sembuf wejscie = {0, -1, 0};
        semop(semid_prom, &wejscie, 1);
    }

    printf("[Przewodnik %d]: Prom odpływa...\n", id_przewodnik);
    sleep(rand() % 5 + 3); // Czas przepłynięcia

    printf("[Przewodnik %d]: Prom dopłynął, grupa wysiada.\n", id_przewodnik);

    // Zwalnianie miejsc na promie
    for (int i = 0; i < liczba_w_grupie; i++) {
        struct sembuf wyjscie = {0, 1, 0};
        semop(semid_prom, &wyjscie, 1);
    }
}

#endif
#ifndef HEADER_H
#define HEADER_H

#define LIMIT 10 // limit osób w parku w danej chwili
#define N 25 // maksymalna ilość osób w parku w ciągu dnia
#define K 4 // liczba kas
//#define M 10 // wielkość grupy odprowadzanej przez przewodnika
//#define P 4 // liczba przewodników

extern int id_turysty[K]; // Tablica przechowująca ID turysty

void *turysta(void *arg);
void *kasjer(void *arg);
//void *przewodnik(void *arg);

#endif
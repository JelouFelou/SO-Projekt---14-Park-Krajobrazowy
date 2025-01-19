#ifndef HEADER_H
#define HEADER_H

#include <pthread.h>

#define LIMIT 40 // limit osób w parku w danej chwili
#define N 100 // maksymalna ilość osób w parku w ciągu dnia
#define M 10 // wielkość grupy odprowadzanej przez przewodnika
#define P 4 // liczba przewodników
#define K 4 // liczba kas

void *kasjer(void *arg);
void *przewodnik(void *arg);
void *turysta(void *arg);

#endif
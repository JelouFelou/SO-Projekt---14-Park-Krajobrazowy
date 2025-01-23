#ifndef HEADER_H
#define HEADER_H

#define LIMIT 20 // limit osób w parku w danej chwili
#define N 50 // maksymalna ilość osób w parku w ciągu dnia
#define K 4 // liczba kas
#define M 10 // wielkość grupy odprowadzanej przez przewodnika
#define P 4 // liczba przewodników

#define RED   "\x1B[31m"
#define GRN   "\x1B[32m"
#define YEL   "\x1B[33m"
#define BLU   "\x1B[34m"
#define RESET "\x1B[0m"

void *turysta(void *arg);
void *kasjer(void *arg);
void *przewodnik(void *arg);

#endif
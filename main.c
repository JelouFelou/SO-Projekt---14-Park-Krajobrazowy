#include<stdlib.h>
#include<stdio.h>
#include<pthread.h>
#include<time.h>

#define N 100 // maksymalna ilość osób danego dnia
#define M 10 // wielkość grupy odprowadzanej przez przewodnika

void *kasjer(void *arg);
void *przewodnik(void *arg);
void *turysta(void *arg);

int main() {
	pthread_t kasjer_thread, przewodnik_thread, turysta_thread[N];
	
	// tworzenie wątków
	pthread_create(&kasjer_thread, NULL, kasjer, NULL);
	pthread_create(&przewodnik_thread, NULL, przewodnik, NULL);
	for (int i = 0; i < N; i++) {
		pthread_create(&turysta_thread[i], NULL, turysta, (void *)(long)i);
	}
	
	// dołączanie wątków
	pthread_join(kasjer_thread, NULL);
	pthread_join(przewodnik_thread, NULL);
	for (int i = 0; i < N; i++) {
		pthread_join(turysta_thread[i], NULL);
	}
	
	return 0;
}
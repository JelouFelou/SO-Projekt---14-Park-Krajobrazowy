#include <stdio.h>
#include <unistd.h>
#include "header.h"

//extern sem_t kasa_zajeta;

void *kasjer(void *arg) {
	int klient = 1;
	while(klient) {
		sem_wait(&kasa_zajeta);
		sleep(1);
		printf("Kasjer: otrzymuje informacje od turysty...\n");
		sleep(1);
		printf("Kasjer: daje zakupiony bilet turyście\n");
		klient--;
		sem_post(&kasa_zajeta);
	}
	return NULL;
}
#include <stdio.h>
#include <unistd.h>
#include "header.h"

void *kasjer(void *arg) {
	int klient = 1;
	while(klient) {
		sleep(1);
		printf("Kasjer: otrzymuje informacje od turysty...\n");
		sleep(1);
		printf("Kasjer: daje zakupiony bilet turyście\n");
		klient--;
	}
	return NULL;
}
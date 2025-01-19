#include <stdio.h>
#include <unistd.h>
#include "header.h"

void *przewodnik(void *arg) {
	int klient = 1;
	while(klient) {
		printf("Przewodnik: Organizuje grupy...\n");
		klient--;
		sleep(2);
	}
	return NULL;
}
#include <stdio.h>
#include <unistd.h>

void *przewodnik(void *arg) {
	while(1) {
		printf("Przewodnik: Organizuje grupy...\n");
		sleep(2);
	}
	return NULL;
}
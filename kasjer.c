#include <stdio.h>
#include <unistd.h>

void *kasjer(void *arg) {
	while(1) {
		printf("Kasjer: sprawdza liczbę osób...\n");
		sleep(1);
	}
	return NULL;
}
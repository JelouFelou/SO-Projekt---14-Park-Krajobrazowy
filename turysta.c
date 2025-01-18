#include <stdio.h>
#include <unistd.h>

void *turysta(void *arg) {
	int id = (int)(long)arg;
	
	printf("Turysta %d: Wchodzi do parku...\n",id);
	sleep(3);
	printf("Turysta %d: Nie widzi nic ciekawego w parku, zatem wychodzi...\n",id);
	
	return NULL;
}
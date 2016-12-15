#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "dsm.h"

int main(int argc, char *argv[]) {
	char * pointer;
	char * current;
	int value;

	pointer = dsm_init(argc, argv);
	current = pointer;

	printf("[%i] COucou, mon adresse de base est : %p\n", DSM_NODE_ID, pointer);

	if (DSM_NODE_ID == 0) {
		current += 16 * sizeof(int);
		value = *((int *) current);
		printf("[%i] valeur de l'entier : %i\n", DSM_NODE_ID, value);
	} else if (DSM_NODE_ID == 1) {
		current += 16 * sizeof(int);
		value = *((int *) current);
		printf("[%i] valeur de l'entier : %i\n", DSM_NODE_ID, value);
	}
	dsm_finalize();
	return 1;
}

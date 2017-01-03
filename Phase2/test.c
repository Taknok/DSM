#include "dsm.h"

int main(int argc, char ** argv){
	printf("Salut\n");
	fflush(stdout);
	dsm_init(argc,argv);


	do{
		short toto = 0;
		while(++toto);// simulation du code pour pouvoir afficher print dans thread
	}while(0);

	dsm_finalize();
}

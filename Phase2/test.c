#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../Phase2/dsm.h"

int main(int argc, char *argv[])
{
	dsm_init(argc,argv);
	printf("salut\n");
   return 0;
}

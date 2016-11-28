#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <poll.h>



//variables globales partagees
#define LENGTH_NAME_MACHINE 100


/* autres includes (eventuellement) */

#define ERROR_EXIT(str) {perror(str);exit(EXIT_FAILURE);}

/* definition du type des infos */
/* de connexion des processus dsm */
typedef struct dsm_proc_conn {
   int rank;
   /* a completer */
   char * name_machine;
}dsm_proc_conn_t;


/* definition du type des infos */
/* d'identification des processus dsm */
typedef struct dsm_proc {
  pid_t pid;
  dsm_proc_conn_t connect_info;
}dsm_proc_t;


void init_tab_dsm_proc(dsm_proc_t * tab_dsm_proc, int tab_size);

int creer_socket(int type, int *port_num);

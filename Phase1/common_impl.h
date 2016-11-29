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

#include <sys/socket.h>
#include <netinet/in.h> //pour gerer l'ip

#include <setjmp.h>

#define TRY do{ jmp_buf env; if( !setjmp(env) ){
#define CATCH } else {
#define END_TRY } }while (0)
#define THROW longjmp(env, 1)

//un peu de booléens
#define FALSE 0
#define TRUE 1






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





//--------------------------------------------------------------------------------------------------------------
//acces reseau



void do_write(int sockfd, char* text);

int do_socket(int domain, int type, int protocol);

void init_serv_addr(int port, struct sockaddr_in * serv_addr);

void do_bind(int sock, struct sockaddr_in adr);

void do_listen(int sock);

int do_accept(int sock, struct sockaddr_in * adr);

int get_port(int sock);

//-------------------------------------------------------------------------------------------------
void sync_child(int * pipe_father, int * pipe_child);

void sync_father(int * pipe_father, int * pipe_child);



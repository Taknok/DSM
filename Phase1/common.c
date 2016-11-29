#include "common_impl.h"



// faire un init des structure pour les malloc
void init_dsm_proc_conn(dsm_proc_conn_t * dsm_proc_conn){
	dsm_proc_conn->name_machine = malloc(LENGTH_NAME_MACHINE + 1);
}

void init_tab_dsm_proc(dsm_proc_t * tab_dsm_proc, int tab_size){
	int i = 0;
	for(i = 0; i < tab_size; i++){
		init_dsm_proc_conn(&tab_dsm_proc[i].connect_info);
	}
}

int creer_socket(int prop, int *port_num) 
{
   int fd = 0;
   
   /* fonction de creation et d'attachement */
   /* d'une nouvelle socket */
   /* renvoie le numero de descripteur */
   /* et modifie le parametre port_num */
   
   return fd;
}

/* Vous pouvez ecrire ici toutes les fonctions */
/* qui pourraient etre utilisees par le lanceur */
/* et le processus intermediaire. N'oubliez pas */
/* de declarer le prototype de ces nouvelles */
/* fonctions dans common_impl.h */



//---------------------------------------------------------------------------------------------------------------------
//Accès réseau

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void do_write(int sockfd, char* text){
    while(send(sockfd, text, strlen(text), 0) == -1) {
            perror("erreur envoie\n");
    }
}

int do_socket(int domain, int type, int protocol){
    int sockfd;
    int yes = 1;
    //create the socket
    sockfd = socket(domain, type, protocol);

    //check for socket validity
    if( sockfd == -1 ) {
            perror("erreur a la creation de la socket");
    }

    // set socket option, to prevent "already in use" issue when rebooting the server right on
    int option = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

    if ( option == -1)
            error("ERROR setting socket options");

    return sockfd;
}

void init_serv_addr(int port, struct sockaddr_in * serv_addr){

    //clean the serv_add structure
    memset(serv_addr, 0, sizeof(serv_addr));

    //cast the port from a string to an int
    //portno = atoi(port); osef car on a mis dans une constante ici coté serveur

    //internet family protocol
    serv_addr->sin_family = AF_INET;

    //we bind to any ip form the host
    serv_addr->sin_addr.s_addr = htonl(INADDR_ANY);

    //we bind on the tcp port specified
    serv_addr->sin_port = htons(port);

}

void do_bind(int sock, struct sockaddr_in adr){

    int retour = bind(sock, (struct sockaddr *) &adr, sizeof(adr));
    if( retour == -1 ) {
        printf("%i\n",errno );
        perror("erreur lors du bind");
    }
}

void do_listen(int sock){
    if( listen( sock, SOMAXCONN) == -1 ) {
        perror("erreur lors de l'ecoute");
        exit(EXIT_FAILURE);
    }
}

int do_accept(int sock, struct sockaddr_in * adr){
    int addrlen=sizeof(adr);
    int new_sock=accept(sock, (struct sockaddr *) &adr,&addrlen);
    if(new_sock==-1)
    printf("Desole, je ne peux pas accepter la session TCP\n");
    else
    printf("connection      : OK\n");
    return new_sock;

}
//-----------------------------------------------------------------------------------------------------------------


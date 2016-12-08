#include "common_impl.h"


int main(int argc, char **argv) {
	if (argc < 5) {
		fprintf(stderr, "Erreur nb arguments");
	}
	/* processus intermediaire pour "nettoyer" */
	/* la liste des arguments qu'on va passer */
	/* a la commande a executer vraiment */

	/* creation d'une socket pour se connecter au */
	/* au lanceur et envoyer/recevoir les infos */
	/* necessaires pour la phase dsm_init */
	struct sockaddr_in sock_host;
	int sock;

	char buffer[BUFFER_SIZE];
	memset(buffer, 0, BUFFER_SIZE);

	//get address info from the server
	char* serv_ip = get_ip(argv[2]);
	//printf("###%s\n", serv_ip);

	//get port
	int serv_port = atoi(argv[1]);

	//get the socket
	sock = do_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//connect to remote socket
	sock_host = do_connect(sock, sock_host, serv_ip, serv_port);

	char hostname[100];
	gethostname(hostname, 99);

	/* Creation de la socket d'ecoute pour les */
	/* connexions avec les autres processus dsm */
	struct sockaddr_in serv_addr;
	int lst_sock = do_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	init_serv_addr(0, &serv_addr);
	do_bind(lst_sock, serv_addr);
	do_listen(lst_sock);

	int port = get_port(lst_sock);

	/* Envoi du nom de machine au lanceur */
	sprintf(buffer, "%s<name>%s</name>", buffer, hostname);

	/* Envoi du numero de port au lanceur */
	/* pour qu'il le propage à tous les autres */
	/* processus dsm */
	sprintf(buffer, "%s<port>%i</port>", buffer, port);

	/* ENVOIE */
	do_write(sock, buffer);

	/* ECOUTE */
//	int sock_recv = 0;
//	int nb_procs = atoi(argv[3]);
//
//
////	sock_recv = accept(lst_sock, NULL, NULL);
//	//initialisation du buffer
//	char * buffer_sock = (char *) malloc(
//	BUFFER_SIZE * 3 * nb_procs * sizeof(char)); //3 car 3 valeurs dans un proc
//
//	int retour_client = do_read(buffer_sock, sock_recv);
//	printf("%s\n",buffer_sock);
//	Client liste_client[nb_procs];
//	int i;
//
//
//	int num_procs;
//	int actual_proc=deserialize(buffer_sock, liste_client, &num_procs);
//	printf("%i\n",actual_proc);
//	printf("%i\n", num_procs);
//
//	for (i = 0; i < nb_procs; ++i) {
//		printf("%s\n", liste_client[i].name);
//		printf("%i\n", liste_client[i].port_client);
//		printf("%i\n", liste_client[i].num_client);
//
//	}
//
//	memset(buffer_sock,0,BUFFER_SIZE * 3 * nb_procs * sizeof(char));
//
//	do_read(buffer_sock, sock_recv);
//	printf("%s\n",buffer_sock);

	fflush(stdout);

	/* on execute la bonne commande */
	execvp(argv[4], &argv[4]);
	return 0;
}

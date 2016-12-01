#include "common_impl.h"

char * get_ip(char * hostname) {
	struct hostent * serv = gethostbyname(hostname);
	struct in_addr ** addr_list;
	addr_list = (struct in_addr **) serv->h_addr_list;
	return inet_ntoa(*addr_list[0]);
}

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


	//get port
	int port = atoi(argv[1]);

	//get the socket
	sock = do_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	//connect to remote socket
	sock_host = do_connect(sock, sock_host, serv_ip, port);

	char * hostname;
	gethostname(hostname, 100);
	/* Envoi du nom de machine au lanceur */
	sprintf(buffer, "<name>%s</name>", hostname);
	/* Envoi du pid au lanceur */

	do_write(sock, buffer);

	/* Creation de la socket d'ecoute pour les */
	/* connexions avec les autres processus dsm */

	/* Envoi du numero de port au lanceur */
	/* pour qu'il le propage à tous les autres */
	/* processus dsm */

	/* on execute la bonne commande */
	execlp(argv[3], argv[4], NULL);
	return 0;
}

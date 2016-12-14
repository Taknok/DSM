#include "common_impl.h"

// faire un init des structure pour les malloc
void init_dsm_proc_conn(dsm_proc_conn_t * dsm_proc_conn) {
	dsm_proc_conn->name_machine = malloc(LENGTH_NAME_MACHINE + 1);
}

void init_tab_dsm_proc(dsm_proc_t * tab_dsm_proc, int tab_size) {
	int i = 0;
	for (i = 0; i < tab_size; i++) {
		init_dsm_proc_conn(&tab_dsm_proc[i].connect_info);
	}
}

int creer_socket(int prop, int *port_num) {
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
void error(const char *msg) {
	perror(msg);
	exit(1);
}

void do_write(int sockfd, char* text) {
	int offset = 0;
	int nbytes = 0;
	int sent = 0;
	while ((sent = send(sockfd, text + offset, strlen(text), 0)) > 0 || (sent == -1 && errno == EINTR) ) {
		if (sent > 0) {
			offset += sent;
			nbytes -= sent;
		}
	}


//	while (send(sockfd, text, strlen(text), 0) == -1) {
//		perror("erreur envoie\n");
//	}
}

int do_socket(int domain, int type, int protocol) {
	int sockfd;
	int yes = 1;
	//create the socket
	sockfd = socket(domain, type, protocol);

	//check for socket validity
	if (sockfd == -1) {
		perror("erreur a la creation de la socket");
	}

	// set socket option, to prevent "already in use" issue when rebooting the server right on
	int option = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes,
			sizeof(int));

	if (option == -1)
		error("ERROR setting socket options");

	return sockfd;
}

void init_serv_addr(int port, struct sockaddr_in * serv_addr) {

	//clean the serv_add structure
	memset(serv_addr, 0, sizeof(*serv_addr));

	//cast the port from a string to an int
	//portno = atoi(port); osef car on a mis dans une constante ici coté serveur

	//internet family protocol
	serv_addr->sin_family = AF_INET;

	//we bind to any ip form the host
	serv_addr->sin_addr.s_addr = htonl(INADDR_ANY);

	//we bind on the tcp port specified
	serv_addr->sin_port = htons(port);

}

void do_bind(int sock, struct sockaddr_in adr) {

	int retour = bind(sock, (struct sockaddr *) &adr, sizeof(adr));
	if (retour == -1) {
		printf("%i\n", errno);
		perror("erreur lors du bind");
	}
}

void do_listen(int sock) {
	if (listen(sock, SOMAXCONN) == -1) {
		perror("erreur lors de l'ecoute");
		exit(EXIT_FAILURE);
	}
}

int do_accept(int sock, struct sockaddr_in * adr) {
	socklen_t addrlen = sizeof(adr);
	int new_sock = accept(sock, (struct sockaddr *) &adr, &addrlen);
	if (new_sock == -1)
		printf("Desole, je ne peux pas accepter la session TCP\n");
	else
		printf("connection      : OK\n");
	return new_sock;

}

int get_port(int sock) {
	int port = 0;
	struct sockaddr_in sin;
	socklen_t len = sizeof(sin);
	if (getsockname(sock, (struct sockaddr *) &sin, &len) == -1) {
		perror("getsockname");
	} else {
		port =  ntohs(sin.sin_port);
	}
	return port;
}

char * get_ip(char * hostname) {
	struct hostent * serv = gethostbyname(hostname);
	struct in_addr ** addr_list;
	addr_list = (struct in_addr **) serv->h_addr_list;
	return inet_ntoa(*addr_list[0]);
}

int do_read(char * buffer, int lst_sock) {
	memset(buffer, 0, BUFFER_SIZE); //on s'assure d'avoir des valeurs nulles dans le buff
	int length_r_buff = recv(lst_sock, buffer, BUFFER_SIZE - 1, 0);

	if (length_r_buff < 0) {
		printf("erreur rien n'a été recu\n");
	} else {
		buffer[length_r_buff] = '\0';
	}
	return length_r_buff;
}

struct sockaddr_in do_connect(int sock, struct sockaddr_in sock_host, char* hostname, int port){
    //reinitialise la memoire
    memset(&sock_host, 0, sizeof(sock_host));

    sock_host.sin_family = AF_INET;
    inet_aton(hostname, &sock_host.sin_addr);
    sock_host.sin_port = htons(port);

    //check de l'erreur
    while(connect(sock, (struct sockaddr *) &sock_host, sizeof(sock_host)) == -1) {
        perror("erreur connect");
//        printf("%s , %i\n", hostname, port);
    }
    return sock_host;
}

//-----------------------------------------------------------------------------------------------------------------

void sync_child(int * pipe_father, int * pipe_child) {
	char sync = 'c';

	close(pipe_father[1]);
	read(pipe_father[0], NULL, sizeof(char));
	close(pipe_child[0]);
	write(pipe_child[1], &sync, sizeof(char));
}

void sync_father(int * pipe_father, int * pipe_child) {
	char sync = 'c';

	close(pipe_father[0]);
	write(pipe_father[1], &sync, sizeof(char));
	close(pipe_child[1]);
	read(pipe_child[0], NULL, sizeof(char));
}

//-----------------------------------------------------------------------------------------------------------------
char * str_extract(char * str, char * p1, char * p2){

	const char * match1 = strstr(str, p1) + strlen(p1);
    const char * match2 = strstr(match1, p2);
    size_t len = match2 - match1;
    char * res = (char*)malloc(sizeof(char)*(len+1));
    strncpy(res, match1, len);
    res[len] = '\0';
    return res;
}

int deserialize(char * serialized, Client * liste_client, int nb_procs) {
	int i;
	char * tmp_seria = serialized;
	char * actual_proc_str = str_extract(tmp_seria, "<actual_proc>", "</actual_proc>");
	int actual_proc=atoi(actual_proc_str);

	for (i = 0; i < nb_procs; ++i) {
		char * machine = str_extract(tmp_seria, "<machine>", "</machine>");

		char * name = str_extract(machine, "<name>", "</name>");
		char * port = str_extract(machine, "<port>", "</port>");
		char * rank = str_extract(machine, "<rank>", "</rank>");

		strcpy(liste_client[i].name, name);
		liste_client[i].port_client = atoi(port);
		liste_client[i].num_client = atoi(rank);
		tmp_seria = strstr(tmp_seria, "</machine>"); //on repointe a une machine d'apres
	}
	return actual_proc;
}

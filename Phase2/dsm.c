#include "dsm.h"
#include "common_impl.h"

int DSM_NODE_NUM; /* nombre de processus dsm */
int DSM_NODE_ID; /* rang (= numero) du processus */

Client * liste_client = NULL;
sem_t sem;

/* indique l'adresse de debut de la page de numero numpage */
static char *num2address(int numpage) {
	char *pointer = (char *) (BASE_ADDR + (numpage * (PAGE_SIZE)));

	if (pointer >= (char *) TOP_ADDR) {
		fprintf(stderr, "[%i] Invalid address !\n", DSM_NODE_ID);
		return NULL;
	} else
		return pointer;
}

/* indique la page de l'adresse donnee*/
static int address2num(void * pageaddr) {
	long int numpage = ((long int) (pageaddr - BASE_ADDR)) / PAGE_SIZE;

	if (numpage > PAGE_NUMBER) {
		fprintf(stderr, "[%i] Invalid num page !\n", DSM_NODE_ID);
		return 0;
	} else
		return numpage;
}

/* fonctions pouvant etre utiles */
static void dsm_change_info(int numpage, dsm_page_state_t state,
		dsm_page_owner_t owner) {
	if ((numpage >= 0) && (numpage < PAGE_NUMBER)) {
		if (state != NO_CHANGE)
			table_page[numpage].status = state;
		if (owner >= 0)
			table_page[numpage].owner = owner;
		return;
	} else {
		fprintf(stderr, "[%i] Invalid page number !\n", DSM_NODE_ID);
		return;
	}
}

static dsm_page_owner_t get_owner(int numpage) {
	return table_page[numpage].owner;
}

static dsm_page_state_t get_status(int numpage) {
	return table_page[numpage].status;
}

/* Allocation d'une nouvelle page */
static void dsm_alloc_page(int numpage) {
	char *page_addr = num2address(numpage);
	mmap(page_addr, PAGE_SIZE, PROT_READ | PROT_WRITE,
	MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
	return;
}

/* Changement de la protection d'une page */
static void dsm_protect_page(int numpage, int prot) {
	char *page_addr = num2address(numpage);
	mprotect(page_addr, PAGE_SIZE, prot);
	return;
}

static void dsm_free_page(int numpage) {
	char *page_addr = num2address(numpage);
	munmap(page_addr, PAGE_SIZE);
	return;
}

static void reception_page(int numpage, int sock) {
	char * buffer_recp = malloc(PAGE_SIZE * sizeof(char));

	int retour;
	do {
		retour = recv(sock, buffer_recp, PAGE_SIZE, MSG_WAITALL);
	} while ((-1 == retour) && (errno == EINTR));

	dsm_alloc_page(numpage);
	dsm_change_info(numpage, WRITE, DSM_NODE_ID);
	void * pointeur_debut_page = num2address(numpage);
	memcpy(pointeur_debut_page, buffer_recp, PAGE_SIZE);

	printf("recp %i : '%s' - %i\n", DSM_NODE_ID, buffer_recp, retour);
	fflush(stdout);

	free(buffer_recp);

	printf("J'ai map alléluia ! \n");
	fflush(stdout);
}

static int dsm_send(int dest, void *buf, size_t size) {
	/* a completer */
	int offset = 0;
	int nbytes = 0;
	int sent = 0;
	while ((sent = send(dest, buf + offset, size, 0)) > 0
			|| (sent == -1 && errno == EINTR)) {
		if (sent > 0) {
			offset += sent;
			nbytes -= sent;
		}
	}
	return sent;
}

static void envoie_page(int id, int numpage) {
	char * pointeur_debut_page = num2address(numpage);

	char * mini_buf = malloc(BUFFER_SIZE * sizeof(char));
	sprintf(mini_buf, "<numpage>%i</numpage>", id, numpage); //pour update de la liste des pages et owner
	int retour;
	do{
		retour = send(liste_client[id].sock_twin, mini_buf, BUFFER_SIZE, 0);
	}while ((-1 == retour) && (errno == EINTR));

	do {
		retour = recv(liste_client[id].sock_twin, mini_buf, BUFFER_SIZE, 0);
	} while ((-1 == retour) && (errno == EINTR));

	free(mini_buf);

	int sent;
	do{
		sent = send(liste_client[id].sock_twin, pointeur_debut_page, PAGE_SIZE,0);
	}while ((-1 == sent) && (errno == EINTR));

	printf(">>>>>>>>>>send %i\n", sent);
	fflush(stdout);
	if (sent == -1) {
		fprintf(stderr, "echec de l'envoie de la page : %i sur %i\n", sent,
		PAGE_SIZE);
		fflush(stderr);
	} else {
		dsm_free_page(numpage);
	}
}

static void *dsm_comm_daemon(void *arg) {
	printf("begin thread %i\n", DSM_NODE_ID);
	fflush(stdout);

	struct pollfd fds[DSM_NODE_NUM];
	memset(fds, 0, sizeof(fds));
	int num_fds = 0;
	/*Ajout du out et err au poll*/
	int j = 0;
	for (j = 0; j < DSM_NODE_NUM; j++) {
		fds[j].fd = liste_client[j].sock_twin;
//		printf("ssssssssssoccccccccccccckkkkkkk id %i  %i\n", DSM_NODE_ID,fds[j].fd);
//		fflush(stdout);
		fds[j].events = POLLIN;
		num_fds++;
	}

	for (;;) {

		//initialisation de la structure pollfd
		/* + ecoute effective */

		if (!poll(fds, num_fds, -1)) {
			perror("  problème sur le poll() ");
			break;
		}

		int z = 0;
		for (z = 0; z < DSM_NODE_NUM; z++) {
			if (fds[z].revents == 0) { //si ya aucun evenement on passe au suivant
				continue;
			}

			char * buffer_sock = (char *) malloc(PAGE_SIZE * sizeof(char));

			sleep(0.1);
			memset(buffer_sock, 0, PAGE_SIZE); //on s'assure d'avoir des valeurs nulles dans le buff
			int length_r_buff;
			do {
				length_r_buff = recv(fds[z].fd, buffer_sock, PAGE_SIZE, 0);
			}while ((-1 == length_r_buff) && (errno == EINTR));
			if (length_r_buff < 0) {
				printf("erreur rien n'a été recu deamon\n");
				perror("error read thread ");
			}

			printf("Thread %i recv on sock %i : '%s' - %i\n", DSM_NODE_ID,
					fds[z].fd, buffer_sock, length_r_buff);
			fflush(stdout);

			if (25 < strlen(buffer_sock) && strlen(buffer_sock) < 40) {
				char * id_str = str_extract(buffer_sock, "<id>", "</id>");
				int id = atoi(id_str);

				char * numpage_str = str_extract(buffer_sock, "<numpage>",
						"</numpage>");
				int numpage = atoi(numpage_str);

				printf("debut envoie page %i  \n", DSM_NODE_ID);
				fflush(stdout);

				envoie_page(id, numpage);

			} else if (strlen(buffer_sock) >= 3) { //pour eviter que le "ok" envoyé repasse ici
				printf("debut reception page %i  \n", DSM_NODE_ID);
				fflush(stdout);

				char * numpage_str = str_extract(buffer_sock, "<numpage>",
						"</numpage>");
				int numpage = atoi(numpage_str);

				int retour;
				do{
					retour = send(fds[z].fd, "ok", 3, 0); //message servant d'acquitement pour la reception de la page
				}while ((-1 == retour) && (errno == EINTR));

				reception_page(numpage, fds[z].fd);
				sem_post(&sem);
			}
			free(buffer_sock);
		}

//		printf("[%i] Waiting for incoming reqs \n", DSM_NODE_ID);
//		sleep(2);
	}
}

static int dsm_recv(int from, void *buf, size_t size) {
	/* a completer */
	memset(buf, 0, size); //on s'assure d'avoir des valeurs nulles dans le buff
	int length_r_buff;
	do{
		length_r_buff = recv(from, buf, size, 0);
	}while ((-1 == length_r_buff) && (errno == EINTR));

	if (length_r_buff < 0) {
		printf("erreur rien n'a été recu lors reception page\n");
	}
	return length_r_buff;
}

static void dsm_handler(int numpage) {
	/* A modifier */

	dsm_page_owner_t owner = get_owner(numpage);
	int sock = liste_client[owner].sock_twin;
	char * buffer_send = malloc(BUFFER_SIZE * sizeof(char));
	snprintf(buffer_send, BUFFER_SIZE, "<id>%i</id><numpage>%i</numpage>",
			DSM_NODE_ID, numpage);

//	printf("%i  mmmmmmmmmmmmm %i\n", DSM_NODE_ID, sock);
//	fflush(stdout);

	int retour;
	do {
		retour = send(sock, buffer_send, BUFFER_SIZE, 0); //demande de la page
	}while ((-1 == retour) && (errno == EINTR));

	if (retour == -1) {
		perror("erreur send page request\n");
		fflush(stderr);
	}
	free(buffer_send);

//		printf("[%i] FAULTY  ACCESS !!! \n", DSM_NODE_ID);
//		abort();
}

/* traitant de signal adequat */
static void segv_handler(int sig, siginfo_t *info, void *context) {
	/* A completer */
	/* adresse qui a provoque une erreur */
	void *addr = info->si_addr;
	/* Si ceci ne fonctionne pas, utiliser a la place :*/
	/*
	 #ifdef __x86_64__
	 void *addr = (void *)(context->uc_mcontext.gregs[REG_CR2]);
	 #elif __i386__
	 void *addr = (void *)(context->uc_mcontext.cr2);
	 #else
	 void  addr = info->si_addr;
	 #endif
	 */
	/*
	 pour plus tard (question ++):
	 dsm_access_t access  = (((ucontext_t *)context)->uc_mcontext.gregs[REG_ERR] & 2) ? WRITE_ACCESS : READ_ACCESS;
	 */
	/* adresse de la page dont fait partie l'adresse qui a provoque la faute */

	void *page_addr = (void *) (((unsigned long) addr) & ~(PAGE_SIZE - 1));

	if ((addr >= (void *) BASE_ADDR) && (addr < (void *) TOP_ADDR)) {
		printf("%i ******* SEGFAULT DE PAGE %p\n", DSM_NODE_ID, addr);
		fflush(stdout);
		int numpage = address2num(page_addr);
		dsm_handler(numpage);
		sem_wait(&sem);
	} else {
		printf("%i ****** SEGFAULT normal boulet\n", DSM_NODE_ID);
		fflush(stdout);
		/* SIGSEGV normal : ne rien faire*/
	}
}

/* Seules ces deux dernieres fonctions sont visibles et utilisables */
/* dans les programmes utilisateurs de la DSM                       */
char *dsm_init(int argc, char **argv) {
	struct sigaction act;
	int index;

	/* ECOUTE */
	int sock_recv = 0;
	int lst_sock = 4; // car le descripteur de lst_sock dans wrap vaut 4;

	do {
		sock_recv = accept(lst_sock, NULL, NULL); //accpete le proc pere
	} while ((-1 == sock_recv) && (errno == EINTR));
	//On regarde la taille de l'envoie (pour le malloc)
	sleep(1); //pas très propre mais si on ne le met pas, possibilité de compter la taille avant que le send soit terminé donc chaine trop petite au final
	int count;
	ioctl(sock_recv, FIONREAD, &count);

	// récup de toute la chaine de caractère
	char * buffer_sock = (char *) malloc(count * sizeof(char)); //3 car 3 valeurs dans un proc
	int retour_client = do_read(buffer_sock, sock_recv);

	/* reception du nombre de processus dsm envoye */
	/* par le lanceur de pr	printf("bonnn");ogrammes (DSM_NODE_NUM)*/
	char * num_proc = str_extract(buffer_sock, "<num_proc>", "</num_proc>");
	int nb_procs = atoi(num_proc);
	DSM_NODE_NUM = nb_procs;

//	printf("<<<<<<<<<<<<<<<< %s \n", buffer_sock);
//	fflush(stdout);

	liste_client = malloc(nb_procs * sizeof(*liste_client));
	int i;
	/* reception de mon numero de processus dsm envoye */
	/* par le lanceur de programmes (DSM_NODE_ID)*/
	/* reception des informations de connexion des autres */
	/* processus envoyees par le lanceur : */
	/* nom de machine, numero de port, etc. */
	int actual_proc = deserialize(buffer_sock, liste_client, nb_procs);
	free(buffer_sock);
	DSM_NODE_ID = actual_proc;

//   	printf("########%i\n",actual_proc);
////   	printf("%i\n", num_procs);
//
//   	for (i = 0; i < nb_procs; ++i) {
//   		printf("%s\n", liste_client[i].name);
//   		printf("%i\n", liste_client[i].port_client);
//   		printf("%i\n", liste_client[i].num_client);
//
//   	}
	fflush(stdout);

	/* initialisation des connexions */
	/* avec les autres processus : connect/accept */
	/*acceptation*/
	int nbr_proc_accept = 0;
	while (nbr_proc_accept < actual_proc) {
		do {
			liste_client[nbr_proc_accept].sock_twin = accept(lst_sock, NULL,
			NULL);
		} while ((-1 == liste_client[nbr_proc_accept].sock_twin)
				&& (errno == EINTR));
		printf("%i: %s a accepté : %i %s\n", actual_proc,
				liste_client[actual_proc].name,
				liste_client[nbr_proc_accept].sock_twin,
				liste_client[nbr_proc_accept].name);
		fflush(stdout);
		nbr_proc_accept++;
	}

	/*connection*/
	int sock;
	struct sockaddr_in sock_host;
	int to_connect = nb_procs;
	while (to_connect - 1 > actual_proc) {
		sock = do_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		liste_client[to_connect - 1].sock_twin = sock;
		char * ip = get_ip(liste_client[to_connect - 1].name);
		int client_port = liste_client[to_connect - 1].port_client;
		sock_host = do_connect(sock, sock_host, ip, client_port);
		printf("%i: %s a connecté : %i %s\n", actual_proc,
				liste_client[actual_proc].name,
				liste_client[to_connect - 1].sock_twin,
				liste_client[to_connect - 1].name);
		fflush(stdout);
		to_connect--;
	}
	free(liste_client);

	/* Allocation des pages en tourniquet */
	for (index = 0; index < PAGE_NUMBER; index++) {
		if ((index % DSM_NODE_NUM) == DSM_NODE_ID)
			dsm_alloc_page(index);
		dsm_change_info(index, WRITE, index % DSM_NODE_NUM);
	}

	/* mise en place du traitant de SIGSEGV */
	act.sa_flags = SA_SIGINFO;
	act.sa_sigaction = segv_handler;
	sigaction(SIGSEGV, &act, NULL);

	//creation du semaphore
	if (sem_init(&sem, 0, 0) == -1) {
		perror("Erreur creation semaphore : ");
	}

	/* creation du thread de communication */
	/* ce thread va attendre et traiter les requetes */
	/* des autres processus */

	int retour = pthread_create(&comm_daemon, NULL, dsm_comm_daemon, NULL);
	if (retour != 0) {
		perror("erreur creation deamon");
		fflush(stderr);
	}

	int toto = 0; // attente pour pouvoir lancer les threads
	while (++toto)
		;

	/* Adresse de début de la zone de mémoire partagée */
	return ((char *) BASE_ADDR);
}

void dsm_finalize(void) {
	/* fermer proprement les connexions avec les autres processus */
//	int j = 0;
//	for (j = 0; j < DSM_NODE_NUM; j++) {
//		close(liste_client[j].sock_twin);
//	}
//
//	close(4); //c'est la sock du dsm exe
	/* terminer correctement le thread de communication */
	/* pour le moment, on peut faire : */
	pthread_cancel(comm_daemon);

	return;
}


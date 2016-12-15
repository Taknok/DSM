#include <unistd.h>
#include <signal.h>

#include "common_impl.h"

/* variables globales */
int DSM_NODE_NUM = 0;
int ARG_MAX_SIZE = 100;

//char * PATH_WRAP = "~/C/DSM/Phase2/bin/dsmwrap";
char * PATH_WRAP = "~/personnel/C/Semestre_7/DSM/Phase2/bin/dsmwrap";

/* un tableau gerant les infos d'identification */
/* des processus dsm */
dsm_proc_t *proc_array = NULL;

/* le nombre de processus effectivement crees */
volatile int num_procs_creat = 0;

void usage(void) {
	fprintf(stdout, "Usage : dsmexec machine_file executable arg1 arg2 ...\n");
	fflush(stdout);
	exit(EXIT_FAILURE);
}

//traitant pour gérer les fils zombi
void sigchld_handler(int sig) {
	pid_t pid;
	int status;

	while ((pid = waitpid(-1, &status, WNOHANG)) == -1) /* suppression du fils zombi */
	{
		perror("wait handler ");

	}
	printf("Prise en compte du fils : %d\n", pid);
}

//compte les lignes du fichier machine_file
int count_line(FILE * FP) {
	int nb_line = 0;
	int c;

	while ((c = getc(FP)) != EOF) { //passage par tout les caractères du fichier
		if (c == '\n') {
			nb_line++;
		}
	}
	fseek(FP, 0, SEEK_SET);
	return nb_line;
}



int main(int argc, char *argv[]) {
	if (argc < 3) {
		usage();
	} else {
		pid_t pid;
		int num_procs = 0;

		/* Mise en place d'un traitant pour recuperer les fils zombies*/
		struct sigaction custom_sigchild;
		memset(&custom_sigchild, 0, sizeof(struct sigaction));
		custom_sigchild.sa_handler = sigchld_handler;
		sigaction(SIGCHLD, &custom_sigchild, NULL);
		/* lecture du fichier de machines */
		/* 1- on recupere le nombre de processus a lancer */

		FILE * FP = fopen(argv[1], "r");
		if (!FP) {
			printf("Impossible d'ouvrir le fichier\n");
			exit(-1);
		}

		DSM_NODE_NUM = count_line(FP);

		/* 2- on recupere les noms des machines : le nom de */
		/* la machine est un des elements d'identification */
		dsm_proc_t tab_dsm_proc[DSM_NODE_NUM];
		init_tab_dsm_proc(tab_dsm_proc, DSM_NODE_NUM);
		struct client_t liste_client[DSM_NODE_NUM];

		int i = 0;
		int r;
		size_t len = 0;
		while ((r = getline(&(tab_dsm_proc[i].connect_info.name_machine), &len,
				FP)) != -1) {

			if (errno != 0) {
				perror("Erreur lors de la lecture des noms de machines");
				errno = 0;
			}
			// remplacement du caractère de fin de ligne par \0
			tab_dsm_proc[i].connect_info.name_machine[strlen(
					tab_dsm_proc[i].connect_info.name_machine) - 1] = '\0';
			tab_dsm_proc[i].connect_info.rank = i;
			liste_client[i].num_client = i;
			i++;
		}

		//----------------------------------------------------------------------------------------
		//----------------------------------------------------------------------------------------------
		/* creation #include <poll.h>
		 * de la socket d'ecoute */
		struct sockaddr_in serv_addr;

		int lst_sock = do_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		init_serv_addr(0, &serv_addr);
		do_bind(lst_sock, serv_addr);
		do_listen(lst_sock);

		int port = get_port(lst_sock);
		// récupération du nom de la machine
		char hostname[1024];
		gethostname(hostname, 1023);

		/* creation des fils */
		num_procs = DSM_NODE_NUM;
		int pipe_out[num_procs][2];
		int pipe_err[num_procs][2];

		int pipe_father[num_procs][2];
		int pipe_child[num_procs][2];

		for (i = 0; i < num_procs; i++) {
			/* creation du tube pour rediriger stdout */
			/* creation du tube pour rediriger stderr */
			pipe(pipe_out[i]);
			pipe(pipe_err[i]);

			pipe(pipe_father[i]);
			pipe(pipe_child[i]);
			pid = fork();
			if (pid == -1)
				ERROR_EXIT("fork");

			if (pid == 0) { /* fils */

				/* Fermeture des descripteurs de fichiers du pere (copié a cause du fork)*/
				int k = 0;
				for (k = 0; k < i - 1; ++k) {
					close(pipe_out[k][0]);
					close(pipe_out[k][1]);
					close(pipe_err[k][0]);
					close(pipe_err[k][1]);
//					close(pipe_father[k][0]);
//					close(pipe_father[k][1]);
//					close(pipe_child[k][0]);
//					close(pipe_child[k][1]);
				}

				/* redirection stdout */
				close(pipe_out[i][0]);
				dup2(pipe_out[i][1], STDOUT_FILENO);

				/* redirection stderr */
				close(pipe_err[i][0]);
				dup2(pipe_err[i][1], STDERR_FILENO);

				/* Creation du tableau d'arguments pour le ssh */
				//cast du port et ip en chaine de caractères
				char port_str[ARG_MAX_SIZE];
				char dsm_num_str[ARG_MAX_SIZE];
				sprintf(port_str, "%i", port);
				char cwd[ARG_MAX_SIZE];
				if (getcwd(cwd, sizeof(cwd)) == NULL) {
					perror("getcwd() error");
				}

				//PB POSSIBLE AVEC LE REALLOC DE POINTEUR CONSTANT
				int taille = 4 + argc;
				char * newargv[taille + 1];

				newargv[0] = "ssh";
				newargv[1] = tab_dsm_proc[i].connect_info.name_machine;
				newargv[2] = PATH_WRAP;
				newargv[3] = port_str;
				newargv[4] = hostname;
				newargv[5] = cwd;
				newargv[6] = argv[2]; //la commande

				int j = 0;
				for (j = 3; j <= argc; j++) { // met les arguments dans le tableau d'execution du ssh
					newargv[4 + j] = argv[j];
				}
				newargv[taille] = NULL;
//
////				sleep(2); //petit sleep pour voir la syncro
////				printf("<<<<<%i\n", getpid());
//				fflush(stdout);
//
//				//synchronisation pere fils
//				sync_child(pipe_father[i], pipe_child[i]);
//
//				printf("<<<<<<<<<<<<<<<<%i\n", getpid());
//				fflush(stdout);
//
//				/* jump to new prog : */
				execvp("ssh", newargv);
//				execlp("ssh", "ssh", "-Y", "normande", "ls", "-a", NULL);
//				execlp("ssh", "ssh", "montbeliarde", PATH_WRAP, "0", "normande", "arg inutile", "ls", "-a", NULL);
//
//				printf("Une erreur dans le exec\n");
//				fflush(stdout);

			} else if (pid > 0) { /* pere */

//				printf(">>>>>>%i\n", pid);
//				fflush(stdout);

				//synchronisation pere fils
//				sync_father(pipe_father[i], pipe_child[i]);
//				printf(">>>>>>>>>>>>>>>>>>\n");

				/* fermeture des extremites des tubes non utiles */
				close(pipe_out[i][1]);
				close(pipe_err[i][1]);

				num_procs_creat++;
			}
		}

		for (i = 0; i < num_procs; i++) {

			/* on accepte les connexions des processus dsm */
			int new_sd = 0;
			while ((new_sd = accept(lst_sock, NULL, NULL)) == -1) {
				if (EINTR == errno) {
					perror("erreur connexion");
				}
			}

			//initialisation du buffer
			char * buffer_sock = (char *) malloc(BUFFER_SIZE * sizeof(char));
			fflush(stdout);

			int retour_client = do_read(buffer_sock, new_sd);
			/*  On recupere le nom de la machine distante */
			char * client_name = str_extract(buffer_sock, "<name>", "</name>");
			/* On recupere le numero de port de la socket */
			/* d'ecoute des processus distants */
			char * client_port = str_extract(buffer_sock, "<port>", "</port>");

			strcpy(liste_client[i].name, client_name);
			liste_client[i].port_client = atoi(client_port);
			close(new_sd);

			printf("%s\n", liste_client[i].name);
			printf("%i\n", liste_client[i].port_client);
			printf("%i\n", liste_client[i].num_client);

		}

		/* envoi du nombre de processus aux processus dsm*/
		/* envoi des rangs aux processus dsm */
		/* envoi des infos de connexion aux processus */
		char liste_serialized[num_procs * BUFFER_SIZE * sizeof(char) * 3]; //3 car 3 arguments dans la struct
		char send[num_procs * BUFFER_SIZE * sizeof(char) * 3
				+ BUFFER_SIZE * sizeof(char)];
		serialize(liste_client, num_procs, liste_serialized);

		for (i = 0; i < num_procs; ++i) {
			struct sockaddr_in sock_host;
			int sock;
			int client_port;
			char* client_ip;

			memset(send, 0,
					num_procs * BUFFER_SIZE * sizeof(char) * 3
							+ BUFFER_SIZE * sizeof(char));
			client_ip = get_ip(liste_client[i].name);

			client_port = liste_client[i].port_client;

			//get the socket
			sock = do_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
			//connect to remote socket

			sock_host = do_connect(sock, sock_host, client_ip, client_port);
			char num_client_str[BUFFER_SIZE];
			sprintf(num_client_str, "<actual_proc>%i</actual_proc>",
					liste_client[i].num_client);
			strcat(send, num_client_str);
			strcat(send, liste_serialized);

			do_write(sock, send);

			close(sock);
		}

		/* gestion des E/S : on recupere les caracteres */
		/* sur les tubes de redirection de stdout/stderr */
		/* while(1)
		 {
		 je recupere les infos sur les tubes de redirection
		 jusqu'à ce qu'ils soient inactifs (ie fermes par les
		 processus dsm ecrivains de l'autre cote ...)

		 };
		 */

		sleep(2);

		struct pollfd fds[2*num_procs_creat];
		memset(fds, 0, sizeof(fds));
		int num_fds = 0;
		/*Ajout du out et err au poll*/
		int j = 0;
		for (j = 0; j < num_procs; j++) {
			fds[2*j].fd = pipe_out[j][0];
			fds[2*j].events = POLLIN;
			num_fds++;

			fds[2*j+1].fd = pipe_err[j][0];
			fds[2*j+1].events = POLLIN;
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
			for (z = 0; z < 2*num_fds; z++) {
				if (fds[z].revents == 0) { //si ya aucun evenement on passe au suivant
					continue;
				}

//				if (fds[z].revents != POLLIN) { //en cas d'evenement chelou
//					perror("error lecture pipe ");
//					break;
//				}

				int tmp;
				char test;
				while ((tmp = read(fds[z].fd, &test, sizeof(char))) == 1) {
					printf("%c", test);
				}

//				while ((tmp = read(pipe_err[z][0], &test, sizeof(char))) == 1) {
//					printf("%c", test);
//				}

			}
		}

//			for (i = 0; i < num_procs; ++i) {
//				int tmp;
//				char test;
//				while ((tmp = read(pipe_out[i][0], &test, sizeof(char))) == 1) {
//					printf("%c", test);
//				}
//
//				while ((tmp = read(pipe_err[i][0], &test, sizeof(char))) == 1) {
//					printf("%c", test);
//				}
//			}

//
//		/* gestion des E/S : on recupere les caracteres */
//		/* sur les tubes de redirection de stdout/stderr */
//		/* while(1)
//		 {
//		 je recupere les infos sur les tubes de redirection
//		 jusqu'à ce qu'ils soient inactifs (ie fermes par les
//		 processus dsm ecrivains de l'autre cote ...)
//
//		 };
//		 */
//
//		/* on attend les processus fils */
//
//		/* on ferme les descripteurs proprement */
//
//		/* on ferme la socket d'ecoute */

	}
	exit(EXIT_SUCCESS);
}


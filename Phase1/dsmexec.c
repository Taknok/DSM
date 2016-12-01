#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>

#include "common_impl.h"

/* variables globales */
int DSM_NODE_NUM = 0;
int ARG_MAX_SIZE = 100;

char * PATH_WRAP = "~/C/DSM/Phase1/bin/dsmwrap";
//char * PATH_WRAP = "~/personnel/C/Semestre_7/DSM/Phase1/bin/dsmwrap";

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
	int pid;

	if ((pid = wait(NULL)) == -1) /* suppression du fils zombi */
	{
		perror("wait handler ");
		errno = 0;
		return;
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
				/* redirection stdout */
				close(pipe_out[i][0]);
				dup2(pipe_out[i][1], STDOUT_FILENO);

				/* redirection stderr */
				close(pipe_err[i][0]);
				dup2(pipe_err[i][1], STDERR_FILENO);

				/* Creation du tableau d'arguments pour le ssh */
				//cast du port et ip en chaine de caractères
				char port_str[ARG_MAX_SIZE];
				sprintf(port_str, "%i", port);

				//PB POSSIBLE AVEC LE REALLOC DE POINTEUR CONSTANT
				int taille = 4 + argc - 2 + 1;
				char * newargv[taille + 1];

				newargv[0] = "ssh";
				newargv[1] = tab_dsm_proc[i].connect_info.name_machine;
				newargv[2] = PATH_WRAP;
				newargv[3] = port_str;
				newargv[4] = hostname;
				newargv[5] = argv[2]; //la commande

				int j = 0;
				for (j = 3; j <= argc; j++) { // met les arguments dans le tableau d'execution du ssh
					newargv[4 + j - 1] = argv[j];
				}
				newargv[taille] = NULL;

//				sleep(2); //petit sleep pour voir la syncro
//				printf("<<<<<%i\n", getpid());
				fflush(stdout);

				//synchronisation pere fils
				sync_child(pipe_father[i], pipe_child[i]);

//				printf("<<<<<<<<<<<<<<<<%i\n", getpid());
				fflush(stdout);

				/* jump to new prog : */
				execvp("ssh", newargv);
//				execlp("ssh", "ssh", "-Y", "normande", "ls", "-a", NULL);
//				execlp("ssh", "ssh", "montbeliarde", PATH_WRAP, "0", "normande", "ls", "-a", NULL);

				printf("Une erreur dans le exec\n");
				fflush(stdout);

			} else if (pid > 0) { /* pere */

//				printf(">>>>>>%i\n", pid);
				fflush(stdout);

				//synchronisation pere fils
				sync_father(pipe_father[i], pipe_child[i]);
//				printf(">>>>>>>>>>>>>>>>>>\n");

				/* fermeture des extremites des tubes non utiles */
				close(pipe_out[i][1]);
				close(pipe_err[i][1]);

				char buffer[BUFFER_SIZE];
				char buffer_err[BUFFER_SIZE];
				while (read(pipe_out[i][0], buffer, sizeof(buffer)) != 0) {
				}
				while (read(pipe_err[i][0], buffer_err, sizeof(buffer_err)) != 0) {
				}
				printf(buffer);
				printf(buffer_err);
				num_procs_creat++;
			}
		}

		for (i = 0; i < num_procs; i++) {

			//initialisation de la structure pollfd
			/* + ecoute effective */
			int new_sd = 0;
			new_sd = accept(lst_sock, NULL, NULL);

			/* on accepte les connexions des processus dsm */

			char * buffer_sock = (char *) malloc(BUFFER_SIZE * sizeof(char));

			printf("buuuuuuuu\n");
			fflush(stdout);

			int retour_client = do_read(buffer_sock, new_sd);

			printf("Entrée : %s\n", buffer_sock);

			printf("a\n");
			char * client_name = str_extract(buffer_sock, "<name>", "</name>");
			printf("result : %s\n", client_name);


			//initialisation du buffer

			/*  On recupere le nom de la machine distante */

			/* 1- d'abord la taille de la chaine */
			/* 2- puis la chaine elle-meme */

			/* On recupere le pid du processus distant  */

			/* On recupere le numero de port de la socket */
			/* d'ecoute des processus distants */
		}

		/* envoi du nombre de processus aux processus dsm*/

		/* envoi des rangs aux processus dsm */

		/* envoi des infos de connexion aux processus */

		/* gestion des E/S : on recupere les caracteres */
		/* sur les tubes de redirection de stdout/stderr */
		/* while(1)
		 {
		 je recupere les infos sur les tubes de redirection
		 jusqu'à ce qu'ils soient inactifs (ie fermes par les
		 processus dsm ecrivains de l'autre cote ...)

		 };
		 */

		/* on attend les processus fils */

		/* on ferme les descripteurs proprement */

		/* on ferme la socket d'ecoute */
	}
	exit(EXIT_SUCCESS);
}


#include <unistd.h>
#include <signal.h>

#include "common_impl.h"
#include "basic.h"

/* variables globales */
int DSM_NODE_NUM = 0;
int ARG_MAX_SIZE = 100;

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

		FILE * FP = fopen("machine_file", "r");
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
		int read;
		size_t len = 0;
		while ((read = getline(&(tab_dsm_proc[i].connect_info.name_machine),
				&len, FP)) != -1) {

			if (errno != 0) {
				perror("Erreur lors de la lecture des noms de machines");
				errno = 0;
			}
			// remplacement du caractère de fin de ligne par \0
			tab_dsm_proc[i].connect_info.name_machine[strlen(
					tab_dsm_proc[i].connect_info.name_machine)] = '\0';
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
		int port=serv_addr->sin_port ;

		// récupération du nom de la machine
		char hostname[1024];
		gethostname(hostname, 1023);

		/* creation des fils */
		num_procs = DSM_NODE_NUM;
		int pipe_out[num_proc][2];
		int pipe_err[num_proc][2];
		for (i = 0; i < num_procs; i++) {
			/* creation du tube pour rediriger stdout */
			/* creation du tube pour rediriger stderr */
			pipe(pipe_out[i]);
			pipe(pipe_err[i]);


			pid = fork();
			if (pid == -1)
				ERROR_EXIT("fork");

			if (pid == 0) { /* fils */

				/* redirection stdout */
				close(pipe_out[i][0]);
				dup2(pipe_out[i][1],STDOUT_FILENO);

				/* redirection stderr */
				close(pipe_err[i][0]);
				dup2(pipe_err[i][1],STDERR_FILENO);

				/* Creation du tableau d'arguments pour le ssh */
				//cast du port en chaine de caractère
				char port_str[ARG_MAX_SIZE];
				sprintf(port_str, "%i", port);


				//PB POSSIBLE AVEC LE REALLOC DE POINTEUR CONSTANT
				int taille = 4 + argc - 2;
				char newargv[taille][ARG_MAX_SIZE] = [tab_dsm_proc[i].connect_info.name_machine, "dsmwrap", port_str, hostname, argv[2]];
				for(j = 3; j <= argc; j++){
					//newargv = (char**)realloc(newargv, (taille+j)*sizeof(*newargv));
					strcpy(newargv[4+j-2], argv[j]);
				}


				/* jump to new prog : */
				execvp("ssh",newargv);

			} else if (pid > 0) { /* pere */
				/* fermeture des extremites des tubes non utiles */
				close(pipe_out[i][1]);
				close(pipe_err[i][1]);

				num_procs_creat++;
			}
		}

		for (i = 0; i < num_procs; i++) {

			//a vérif
			/* + ecoute effective */
			//int poll(struct pollfd *fds, nfds_t nfds, int délai);
			//struct pollfd poll_fd[DSM_NODE_NUM];
			/* on accepte les connexions des processus dsm */

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


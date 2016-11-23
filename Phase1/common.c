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

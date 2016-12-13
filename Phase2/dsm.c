#include "dsm.h"
#include "common_impl.h"


int DSM_NODE_NUM; /* nombre de processus dsm */
int DSM_NODE_ID;  /* rang (= numero) du processus */ 



/* indique l'adresse de debut de la page de numero numpage */
static char *num2address( int numpage )
{ 
   char *pointer = (char *)(BASE_ADDR+(numpage*(PAGE_SIZE)));
   
   if( pointer >= (char *)TOP_ADDR ){
      fprintf(stderr,"[%i] Invalid address !\n", DSM_NODE_ID);
      return NULL;
   }
   else return pointer;
}

/* fonctions pouvant etre utiles */
static void dsm_change_info( int numpage, dsm_page_state_t state, dsm_page_owner_t owner)
{
   if ((numpage >= 0) && (numpage < PAGE_NUMBER)) {	
	if (state != NO_CHANGE )
	table_page[numpage].status = state;
      if (owner >= 0 )
	table_page[numpage].owner = owner;
      return;
   }
   else {
	fprintf(stderr,"[%i] Invalid page number !\n", DSM_NODE_ID);
      return;
   }
}

static dsm_page_owner_t get_owner( int numpage)
{
   return table_page[numpage].owner;
}

static dsm_page_state_t get_status( int numpage)
{
   return table_page[numpage].status;
}

/* Allocation d'une nouvelle page */
static void dsm_alloc_page( int numpage )
{
   char *page_addr = num2address( numpage );
   mmap(page_addr, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
   return ;
}

/* Changement de la protection d'une page */
static void dsm_protect_page( int numpage , int prot)
{
   char *page_addr = num2address( numpage );
   mprotect(page_addr, PAGE_SIZE, prot);
   return;
}

static void dsm_free_page( int numpage )
{
   char *page_addr = num2address( numpage );
   munmap(page_addr, PAGE_SIZE);
   return;
}

static void *dsm_comm_daemon( void *arg)
{  
   while(1)
     {
	/* a modifier */
	printf("[%i] Waiting for incoming reqs \n", DSM_NODE_ID);
	sleep(2);
     }
//   return;
}

static int dsm_send(int dest,void *buf,size_t size)
{
   /* a completer */
}

static int dsm_recv(int from,void *buf,size_t size)
{
   /* a completer */
}

static void dsm_handler( void )
{  
   /* A modifier */
   printf("[%i] FAULTY  ACCESS !!! \n",DSM_NODE_ID);
   abort();
}

/* traitant de signal adequat */
static void segv_handler(int sig, siginfo_t *info, void *context)
{
   /* A completer */
   /* adresse qui a provoque une erreur */
   void  *addr = info->si_addr;   
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
   void  *page_addr  = (void *)(((unsigned long) addr) & ~(PAGE_SIZE-1));

   if ((addr >= (void *)BASE_ADDR) && (addr < (void *)TOP_ADDR))
     {
	dsm_handler();
     }
   else
     {
	/* SIGSEGV normal : ne rien faire*/
     }
}

/* Seules ces deux dernieres fonctions sont visibles et utilisables */
/* dans les programmes utilisateurs de la DSM                       */
char *dsm_init(int argc, char **argv)
{   
   struct sigaction act;
   int index;


   /* ECOUTE */
   	int sock_recv = 0;
   	int lst_sock=4; // car le descripteur de lst_sock dans wrap vaut 4;

   	sock_recv = accept(lst_sock, NULL, NULL);

   	//On regarde la taille de l'envoie (pour le malloc)
   	sleep(1); //pas très propre mais si on ne le met pas, possibilité de compter la taille avant que le send soit terminé donc chaine trop petite au final
   	int count;
   	ioctl(sock_recv,FIONREAD,&count);

    // récup de toute la chaine de caractère
   	char * buffer_sock=(char *) malloc(count * sizeof(char));  //3 car 3 valeurs dans un proc
   	int retour_client=do_read(buffer_sock,sock_recv);

    /* reception du nombre de processus dsm envoye */
      /* par le lanceur de programmes (DSM_NODE_NUM)*/
   	char * num_proc = str_extract(buffer_sock, "<num_proc>", "</num_proc>");
    int nb_procs=atoi(num_proc);

    printf("<<<<<<<<<<<<<<<< %s \n", buffer_sock);
    fflush(stdout);


   	Client liste_client[nb_procs];
   	int i;
    /* reception de mon numero de processus dsm envoye */
    /* par le lanceur de programmes (DSM_NODE_ID)*/
   	/* reception des informations de connexion des autres */
   	/* processus envoyees par le lanceur : */
   	/* nom de machine, numero de port, etc. */
   	int actual_proc=deserialize(buffer_sock, liste_client, nb_procs);

//   	printf("%i\n",actual_proc);
//   	printf("%i\n", num_procs);
//
//   	for (i = 0; i < nb_procs; ++i) {
//   		printf("%s\n", liste_client[i].name);
//   		printf("%i\n", liste_client[i].port_client);
//   		printf("%i\n", liste_client[i].num_client);
//
//   	}
   
   /* initialisation des connexions */ 
   /* avec les autres processus : connect/accept */
   	int nbr_proc_accept=0;
   	while(nbr_proc_accept<actual_proc){
   		liste_client[nbr_proc_accept].sock_twin=accept(lst_sock, NULL, NULL);
   		nbr_proc_accept++;
   	}

   
   	int sock;
   	struct sockaddr_in sock_host;
   	int to_connect=nb_procs;
   	while(to_connect-1>actual_proc){
   		sock = do_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
   		liste_client[to_connect-1].sock_twin=sock;
   		char * ip= get_ip(liste_client[to_connect-1].name);
   		int client_port=liste_client[to_connect-1].port_client;
   		sock_host = do_connect(sock, sock_host,ip  , client_port );
   		to_connect--;
   	}



   /* Allocation des pages en tourniquet */
   for(index = 0; index < PAGE_NUMBER; index ++){	
     if ((index % DSM_NODE_NUM) == DSM_NODE_ID)
       dsm_alloc_page(index);	     
     dsm_change_info( index, WRITE, index % DSM_NODE_NUM);
   }
   
   /* mise en place du traitant de SIGSEGV */
   act.sa_flags = SA_SIGINFO; 
   act.sa_sigaction = segv_handler;
   sigaction(SIGSEGV, &act, NULL);
   
   /* creation du thread de communication */
   /* ce thread va attendre et traiter les requetes */
   /* des autres processus */
   pthread_create(&comm_daemon, NULL, dsm_comm_daemon, NULL);
   
   /* Adresse de début de la zone de mémoire partagée */
   return ((char *)BASE_ADDR);
}

void dsm_finalize( void )
{
   /* fermer proprement les connexions avec les autres processus */

   /* terminer correctement le thread de communication */
   /* pour le moment, on peut faire : */
   pthread_cancel(comm_daemon);
   
  return;
}


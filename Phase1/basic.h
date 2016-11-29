#ifndef BASIC_H
#define BASIC_H

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h> //pour gerer l'ip
#include <string.h>
#include <errno.h>

#include <setjmp.h>

#define TRY do{ jmp_buf env; if( !setjmp(env) ){
#define CATCH } else {
#define END_TRY } }while (0)
#define THROW longjmp(env, 1)

//un peu de booléens
#define FALSE 0
#define TRUE 1


void do_write(int sockfd, char* text);

int do_socket(int domain, int type, int protocol);

void init_serv_addr(int port, struct sockaddr_in * serv_addr);

void do_bind(int sock, struct sockaddr_in adr);

void do_listen(int sock);

int do_accept(int sock, struct sockaddr_in * adr);

#endif

/** \file comsock.c
       \author Giuseppe Miraglia
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  */

#include <sys/socket.h>
#include <sys/un.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "comsock.h"

enum tf { ZERO = 0, M_UNO = -1, UNO = 1 };
enum chr { TCH = '\0', DOT = '.' };
 
#define MAX_NUM 8		/* max n°cifre pid processo */
#define MAX_BUFF 150

#define ERROR1(A) { errno = A; \
		   return M_UNO; }


#define ERRM1(c) if ( c == M_UNO ) 
#define EXITERR  return M_UNO;

#define EXIT1(fd) { status = errno; \
		   ERRM1 ( closeSocket(fd) ) \
		   		  EXITERR \
			else { \
		   errno = status; \
		   EXITERR }} 

#define EXIT2(fd,path) {  status = errno; \
		   	  ERRM1 ( closeSocket(fd) ) \
		   		  EXITERR \
				else {  \
			  ERRM1 ( unlink(path) ) \
					EXITERR \
			  errno = status; \
			  EXITERR }}


#define EXIT_SR { if ( rd == ZERO ) 	/* nessuno scrittore: nessuna connessione */\
			errno = ENOTCONN; \
		EXITERR }

/**
Clenup handler.
**/

static void cleanup(void *arg){

free(arg);

}

/******************************/

int closeSocket(int s){

ERRM1( close(s) )
	EXITERR

return ZERO;
}

/******************************/

int createServerChannel(char *path){	

struct sockaddr_un address;
int fd_sock, status;

if ( !path )
	ERROR1(EINTR);

if ( (strlen(path) + UNO) > UNIX_PATH_MAX )	/* path troppo lungo ? */
				ERROR1(E2BIG);
	
ERRM1( (fd_sock = socket(AF_UNIX, SOCK_STREAM, ZERO)) )		/* creazione di un socket */	
					EXITERR	

/* inizializzazione indirizzo */

address.sun_family = AF_UNIX;
strcpy(address.sun_path,path);

ERRM1 ( bind(fd_sock, (struct sockaddr *) &address, sizeof(address)) )	/* associazione indirizzo */
							EXIT1(fd_sock); 					

ERRM1 ( listen(fd_sock, SOMAXCONN) )	/* la socket viene messa in condizioni di accettare fino a SOMAXCONN connessioni */
			EXIT2(fd_sock,path)				

return fd_sock;
}


/******************************/

int openConnection(char *path){

int fd_sock, try = ZERO, status;
struct sockaddr_un addr, addr2;

if ( (strlen(path) + UNO) > UNIX_PATH_MAX )
			ERROR1(E2BIG); 


/* creazione socket client su cui si connetterà il SERVER */

ERRM1 ( (fd_sock = socket(AF_UNIX, SOCK_STREAM, ZERO)) )
					EXITERR

addr2.sun_family = AF_UNIX;

sprintf( addr2.sun_path, "./tmp/Client_%d.sck", getpid() );		/* determinazione nome univoco */

ERRM1 ( bind(fd_sock, (struct sockaddr *) &addr2, sizeof(addr2)) )	/* associazione indirizzo */
						EXIT1(fd_sock) 

/* preparazione indirizzo per la connessione al socket del server ( accettazione ) */

addr.sun_family = AF_UNIX;

strcpy(addr.sun_path,path);

/* connetto fd_sock (avente nome "Client_pid.sck" ) con il server */

while ( connect(fd_sock, (struct sockaddr *) &addr, sizeof(addr)) == M_UNO ){	/* tentativi di connessione */
	
							if ( try < NTRIALCONN ){	/* al max NTRIALCONN prima di ritornare */	 
									sleep(UNO);
									try++; }
								else
							EXIT2(fd_sock, addr2.sun_path)	}
					
return fd_sock;
}

/************************************/

int acceptConnection(int s){

int sock_conn;

/* connessione al primo client in coda o attesa bloccante (se non ci sono connessioni pendenti) */

ERRM1( (sock_conn = accept(s, NULL, ZERO)) )
				EXITERR

return sock_conn;
}


/******************************************/
/** Protocollo utilizzato :  TIPO_MEX o LUNGH_MEX o '.' o BUFFER_MEX (concatenazione)

	es: per un msg avente
		- msg->type = MSG_OK
		- msg->length = 5
		- msg->buffer = ciao
	
	otteniamo la stringa mex : 	K5.ciao
**/

int sendMessage(int sc, message_t *msg){

char *helper;
char len[MAX_NUM], mex[MAX_BUFF];
int i, counter;

sprintf(len, "%d", msg->length);

if ( msg->length > ZERO)
	counter = strlen(len) + msg->length + UNO + UNO; 	/* + '.' + type; '\0' compreso in msg->length */

	else 
	counter = 4;					/* type + strlen('0') + '.' + '\0' */ 


mex[ZERO] = msg->type;		/* impacchettamento type */

for ( helper = mex + UNO, i = ZERO; len[i] != TCH ; helper++, i++ )	/* impacchettamento length */
						*helper = len[i];
	
*helper = DOT;
helper++;

if ( msg->length > ZERO ){	/* impacchettamento del buffer, se previsto */
		
		for ( i = ZERO; i < msg->length; i++, helper++ )		/* copre anche il caso username\0pwd\0sock */
						*helper = *((msg->buffer)+i); 
					}
		else 
		*helper = TCH;
		
if ( (i = write(sc, mex, counter)) != counter ){
					if ( i == M_UNO )		/* si assume SIGPIPE ignorato */
						errno = ENOTCONN;
					EXITERR }

return counter;
}

/*********************************************/


int receiveMessage(int sc, message_t *msg){

int rd, i;
char buff[MAX_NUM], app;

if ( (rd = read(sc, (char *) &msg->type, UNO)) <= ZERO ) 		/* letture del tipo di messaggio -- singolo carattere -- */
					 EXIT_SR

for ( i = ZERO; ; i++ ){		/* stima e lettura della lunghezza del messaggio -- n° di cifre variabili delimitate dal '.' */
		
	if ( (rd = read(sc, (char *) &app, UNO)) <= ZERO )
						EXIT_SR

	if ( app != DOT )
		buff[i] = app;
		
		else {
		buff[i] = TCH;
		break;  } }

msg->length = atoi(buff);						/* sistemata la lunghezza del messaggio */

msg->buffer = NULL;
pthread_cleanup_push(cleanup, msg->buffer);

if ( msg->length > ZERO){

	if ( !(msg->buffer = malloc(sizeof(char) * msg->length)) )
							EXITERR

	if ( (rd = read(sc, msg->buffer, msg->length)) != msg->length )		/* lettura dell'intero buffer, lungo msg->length */
						EXIT_SR 

			}

		else
		if ( (rd = read(sc, (char *) &app, UNO)) <= ZERO )	/* tolgo il '\0' rimanente dopo il '.' */
						EXIT_SR

pthread_cleanup_pop(ZERO);

return (rd + strlen(buff) + UNO);
}


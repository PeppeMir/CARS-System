/** \file fun_client.h
       \author Giuseppe Miraglia
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  */

#ifndef __CLNT_H
#define __CLNT_H

#include "comsock.h"
#include <errno.h>
#include <unistd.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

enum bool { true=1, false=0 };
enum special_char { DDOT = ':', TCHR = '\0' }; 
enum retval_error { OK = 1, UNO = 1, ZERO = 0, NEG = -1, SHAREND_EXIT = -2, MEXEXIT = -3, MEMEXIT = -5 };

#define MAXRD 291		/* per la lettura delle richieste da stdin --> (2 * LLABEL) + 32(cifre NPOSTI) + 3(:) */ 
#define CLIENTSCKLBL 25		/* max dimensione nome socket per client: "Client_pid.sck" (standard: pid è di 4-5 cifre) */
#define LLABEL 128		/* max dim label città */					/* invio/ricezione scorretta di un messaggio*/

#define SERVER_SCK "./tmp/cars.sck"	/* socket del server */

#define EXIT_1(A) { perror(A); \
		    _exit(errno); }

#define EXIT_2(A) { fprintf(stderr,A); \
		    _exit(OK); }

#define EXIT_3(a) return a;

#define ERRM1(c) if ( c == NEG ) 

#define RM_SCK closeSocket(fd_sock); \
	       unlink(sck_name);

#define EXIT_TH(A) pthread_exit( (void *) A);

#define ERRMALLOC(c) if ( !c )

/************************************************************ variabili globali ********************************************************************************/

int fd_sock;		/* file descriptor del client-socket */

char psw[LUSERNAME], sck_name[CLIENTSCKLBL], *username;		/* password, socket name e username del client */

pthread_t tid_req, tid_rx;		/* tid del thread che si occupa delle richieste e del thread che manipola le risposte, rispettivamente */
message_t *msgProc;			/* struttura relativa al processo client per l'invio dei messaggi */

/***************************************************************************************************************************************************************/

/**
 Invia il messaggio di LOGIN sul socket creato dal client(fd_sock globale).

\ param proc_pid  il pid del processo client che la chiama (per identificare il socket creato)
\ param msg	struttura da riempire col il messaggio desiderato, impacchettato opportunamente dalla sendMessage ( -- comsock.h )

\ retval 0  se il messaggio è stato inviato correttamente
\ retval -1  in caso di invio nullo o parziale
**/

int sendLogin(int proc_pid, message_t *msg);

/**
 Verifica se il pezzo di stringa [ *str ... pos ] ha dimensione <= LLABEL.

\retval 0  in caso affermativo;
\retval -1  altrimenti.
**/

int count_lbl(char **str, char *pos);

/** 
Funzione di parsing: effettua uno switch sulla stringa passata come argomento e restituisce un carattere dipendentemente dal formato di tale stringa.

\ param str la stringa da esaminare

switch ( type ) case

\ retval n (normal) se si presenta un offerta -- label1:label2:km --
\ retval p (place) se si presenta una richiesta -- %R label1:label2 --
\ retval --> se si presenta una comando di tipo %HELP (h) o %EXIT (s)
\ retval e (error) se si presenta un comando mal formato o con errori di sintassi	
**/

char parse(char *str);

/**
 Funzione che stampa una breve descrizione riguardante il corretto modo di inserire richieste ed offerte da input (in risposta al comando '%HELP') 
**/

void printHelp();

/** 
Invia al server un messaggio contenente l'offerta/richiesta di sharing, impacchettando il contenuto di msg(param) secondo il protocollo di comunicazione stabilito.

\param rd    richiesta specifica, da allocare in msg->buffer;
\param msg   struttura che dovrà contenere il messaggio che sarà a sua volta parametro della sendMessage;
\param type  carattere di codifica richiesta (p), offerta (n) o uscita (s)

\retval 0  in caso di invio totale e corretto del mex;
\retval -1  in caso di errore (sets errno)
**/

int send_rd(char *rd, message_t *msg, char type);

#endif

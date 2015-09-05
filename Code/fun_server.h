/** \file fun_server.h
       \author Giuseppe Miraglia
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  */

#ifndef __SERV_H
#define __SERV_H

#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <pthread.h>
#include "dgraph.h"
#include "shortestpath.h"
#include "comsock.h"

/*******************************************************************************************************************************************/
/********************************************** Strutture e nuovi tipi *********************************************************************/

typedef struct cliente {	/* struttura che definisce gli elementi della coda dei Client connessi */
				/* dimensione massima LUSERNAME ( --comsock.h) */
char *username;			
char *password;
int fd_c;		/* file descriptor del socket associato all'utente */
struct cliente *next;

}archive;

/*********************/

typedef struct citta{		/* struttura che definisce gli estremi di una rotta, in indici (città_arrivo-città_partenza) --- usata per le richieste di share */

char usernm[LUSERNAME];		/* definisce l'username dell' utente che ha inviato la richiesta */
unsigned int start;		/* definisce la città di partenza della richiesta */
unsigned int end;		/* definisce la città di arrivo della richiesta */
struct citta *next;

}req_queue; 

/*********************/

typedef struct offer{		/* struttura che definisce la tipica offerta di un utente */

req_queue *rotta;		/* definisce la rotta dell'offerta (vedi struttura precedente) */
unsigned int posti;		/* numero di posti offerti */
char *rotta_str;		/* puntatore alla stringa contenente la rotta calcolata sui cammini minimi a partire da start */ 
struct offer *next;

}offer_queue;

/*********************/

typedef struct threads {

pthread_t tid;			/* thread identifier */
unsigned int active;		/* stato del thread: 0 non attivo, 1 attivo */
struct threads *next;		/* puntatore all'elemento successivo */

}tid_list;

/*********************/

enum special_char { DDOT = ':', TCHR = '\0', DLR = '$' }; 
enum retval_error { DUE = 2, UNO = 1, ZERO = 0, GENERICERR = -1, MEXEXIT = -2, NOLOGIN = -3, MEMEXIT = -5 };

/*********************/

#define REFUSELOGIN "Login incorrect"
#define REFUSEREQOFF "Richiesta/Offerta ingestibile"

/* NAME */
#define CLIENTSCKLBL 25		/* max dimensione nome socket per client: "Client_pid.sck" (standard: pid è di 4-5 cifre) */
#define LOGFILE "./mgcars.log"
#define DIR "./tmp"
#define SERVER_SCK "./tmp/cars.sck"
#define MAXWR 80

/* EXIT */	    
#define STD_EXIT return GENERICERR;

#define IF_CLOSE(A)  if ( fclose(A) == EOF ) \
				write(DUE,"Errore nella chiusura del file\n", MAXWR);

#define CLOSE_FD fclose(fd1);\
		 fclose(fd2); \
		 free_graph(&graph); \
		 exit(errno);

#define EXIT_1(A){  perror(A); \
		    fclose(fd_log);\
		    closeSocket(fd_sk); \
		    unlink(SERVER_SCK); \
		    CLOSE_FD  }

#define EXIT_2(A) { fprintf(stderr,A); \
		    exit(TRUE); }

#define EXIT_THR(A) pthread_exit( (void *) A);

#define EXITMEX_TH(A) { perror(A); \
		   	if ( msg->buffer )\
				free(msg->buffer);\
		   	EXIT_THR(MEXEXIT) }

#define ERRM1(c)  if ( c == GENERICERR )

#define ERRMALLOC(c) if ( !c ){ \
		perror("Malloc");

#define ALLOCA(var) var = malloc(sizeof(archive)); \
		    var->username = malloc(sizeof(char) * len_u); \
		    var->password = malloc(sizeof(char) * len_p);

/*******************************************************************************************************************************************/
/******************************************************** Variabili globali ****************************************************************/


FILE *fd1, *fd2, *fd_log;		/* strutture per l'apertura dei file per la creazione del grafo e di log */
int fd_sk;				/* fd socket del server */

pthread_t tidm;				/* tid del thread match*/
tid_list *tidw;				/* puntatore alla testa della lista di tid worker */

graph_t *graph;				/* puntatore al grafo */
archive *lista_clienti;			/* puntatore alla lista dei client che si sono connessi almeno una volta */
offer_queue *offerte;			/* puntatore alla lista delle offerte da esaminare */
req_queue *richieste;			/* puntatore alla lista delle richieste da esaminare */

offer_queue **offerte_utilizzate;	/* array di puntatori: riferimenti alle offerte utilizzate dal match per soddisfare una certa richiesta */
char **stringhe_utilizzate;		/* array di puntatori: riferimenti alle stringhe utilizzate dal match per soddisfare una certa sottorichiesta */

volatile sig_atomic_t indexM;		/* variabile che mantiene i secondi attesi dal match nella pre-elaborazione/sincronizzazione */

char *da_trovare_sx;			/* puntatori ausiliari all'algoritmo di calcolo degli accoppiamenti */
char *rotta_richiesta;		

static pthread_mutex_t mutex_list = PTHREAD_MUTEX_INITIALIZER;	/* semaforo per la gestione delle liste di offerte e richieste */

/*********************************************************************************************************************************************/

/**
Dealloca la memoria relativa alla lista dei thread connessi
**/

void svuota_tid_list();

/** 
Dealloca tutta la memoria relativa alle strutture dati contententi le offerte e le richieste pendenti e la lista degli utenti connessi, allocate 
dal processo durante l'esecuzione. 
**/

void svuota_liste();

/**
Aggiunge in coda alla lista dei thread puntata da tid, un elemento di tipo tid_list.

\retval puntatore all'elemento aggiunto, in caso di successo.
\retval NULL  in caso di errore.
**/

tid_list * add_thread();


/** 
Cerca nella lista degli utenti loggati, il file descriptor corrispondente all'username usr.

\param usr  username dell'utente;

\retval fd  il file descriptor associato al socket del cliente;
\retval -1  in caso non ci fosse corrispondenza.
**/

int search_fd_user(char *usr);

/** 
1) Effettua uno switch sul tipo di messaggio da inviare al client, dipendentemente dal parametro type.
2) Il tipo di messaggio selezionato viene impacchettato ed inviato al socket avente fd come file descriptor.     

/param fd   file descriptor del socket;
/param msg  struttura per contenere il messaggio (deve essere allocata fuori) ;
/param type  carattere identificatore del tipo di messaggio (per lo switch);

/retval 0  se tutto è andato a buon fine;
/retval -1 se si è verificato un errore (sets errno).
**/

int SwitchSend( int fd, message_t *msg, char type);

/** 
1) Calcola, per ogni elemento della lista delle offerte e secondo l'algortirmo di dijkstra, il cammino minimo da pun->start a pun->end;

2) in base all'array dei predecessori ottenuto, determina la stringa della rotta e inizializza a questa il valore del puntatore all'interno
   delle rispettive strutture di tipo offer_queue.

\param pun  puntatore alla testa della lista delle offerte;

\retval 0  se tutto è andato a buon fine;
\retval -1 se la rotta non esiste
\retval error number  se si è verificato qualche errore (sets errno).

**/

int piazza_rotte(offer_queue *pun);


/** 
Effettua il login relativo all'utente avente username e password presenti nell'area di memoria puntata da buffer (param).

    Aggiunge un elemento alla lista dei client e ne memorizza la password (se non è già in lista); 
    se invece l'utente è già presente in lista, verifica se c'è match tra le password memorizzata-passata.

\param buffer  buffer contentente username e password del cliente che desidera connettersi ( username\0password )
\param usernm  puntatore al buffer del worker: verrà riempito con il nome del socket dedicato (presente in msg)
\param fd  file descriptor associato al socket del client

\retval 0  se l'autenticazione avviene con successo
\retval -1  in caso di errore

**/

int effettua_login(char *buffer, char **usernm, int fd);

/**
 Ricevuta una richiesta o un offerta (%R CITTA1:CITTA2 oppure CITTA1:CITTA2:N_POSTI), la analizza controllando se le città componenti 
la richiesta/offerta appartengono alla mappa.

In caso affermativo viene accodata; in caso negativo non viene accodata e viene restituito un messaggio di errore.

\param buffer  è il puntatore alla richiesta/offerta da accodare.

\retval 0  tutto ok, richiesta accodata
\retval 1 tutto ok, offerta accodata
\retval -1 (and sets errno)  la richiesta non soddisfa uno dei casi suddetti (EBADMSG) oppure c'è stato un errore nell'accodamento 
\retval MEMEXIT  in caso di errori nell'allocazione della memoria.

**/

int check_and_queues(char *buffer, char *usernm);

/**
 Controlla, all'interno della lista delle richieste, se l username puntato dal puntatore passato è univoco rispetto all'esistenza di altre richieste
relative allo stesso utente (e quindi aventi lo stesso username).

\param usernm  è l'username da ricercare

\retval 1  se l username è univoco: esiste una sola richiesta relativa all'username passato.
\retval >1  se l'username non è univoco: esistono più richieste relative allo stesso username
\retval -1 se c'è stato un errore.
**/

int check_req(char *usernm);

/** 
Modifica la coda delle richieste e quella delle offerte in base alle condizioni 1) e 3):

1) rimuove la richiesta puntata da rq(param) dalla rispettiva coda;
2) controlla se la richiesta che si sta rimuovendo è l'ultima relativa allo stesso utente;
3) rimuove la/le offerta/e - i cui indirizzi sono mantenuti nell'array "offerte_utilizzate" - dalla rispettiva coda;

\param rq  puntatore a puntatore della richiesta da rimuovere;
\param rq_prec  puntatore a puntatore del predecessore della richiesta da rimuovere;
\param i  massimo indice dell'array "offerte_utilizzate" utilizzato (identifica l'ultima cella significativa).

\retval 1  tutto a buon fine, non ci sono più richieste da soddisfare per il client considerato; 
\retval >1 tutto a buon fine, ci sono ancora richieste da soddisfare;
\retval -1 in caso di errore.
**/

int sistema_code(req_queue **rq, req_queue **rq_prec, int i);

/** 
Conta il numero di occorrenze del carattere '$' presenti all'interno della stringa puntata dal puntatore passato alla funzione.

\param rotta_req puntatore alla stringa contenente la rotta della richiesta da esaminare;

\retval numero di occorrenze.
**/

int conta_max_offerte(char *rotta_req);

/** 
Copia lo "scarto destro" della stringa nella lista delle richieste parziali ancora da soddisfare;
    se nella lista è presente una stringa contenuta in quella che stiamo per aggiungere, rimpiazza la prima con la seconda.

\param global  variabile puntatore (globale) che punta alla stringa contenente la rotta (intera) della richiesta;
\param pos  puntatore che delimita la porzione di stringa scartata;

**/

void parzial_save(char *global, char *pos);

/** 
Controlla se la stringa puntata da s(param) è presente nella lista delle offerte pendenti come rotta esatta o sottorotta;

\param s  stringa da confrontare

\retval off_pun  puntatore all'offerta che soddisfa la rotta della richiesta (s), in caso di match;
\retval NULL in caso non esista nessun match.
**/

offer_queue * scan(char *s, offer_queue *off_pun);

/** 
Verifica se una certa richiesta, avente rotta puntata da copia_req(param), è soddisfatta da almeno un offerta (lista offerte pendenti);

in caso affermativo, i puntatori alle offerte che contribuisco a soddisfarla, vengono mantenuti nell'array di puntatori puntato da "offerte_utilizzate", 
da ritenersi attendibile solo in tal caso.

\param copia_req  puntatore alla stringa da ricercare;
\param select  identifica il caso di terminazione della ricorsione: 
			- se == 0 sto ricorrendo sulla stringa alla quale ho "sottratto" una città dal fondo;
			- se == 1 sto ricorrendo sul pezzo di stringa per il quale non c'è stato ancora match.

\param i  indice dell'array delle offerte utilizzato nella chiamata ricorsiva;

\retval i in caso di richiesta soddisfatta, ovvero l'indice massimo dell'array usato per mantenere i riferimenti alle offerte utilizzate per soddisfare la richiesta;
\retval -1 se non ho trovato alcuna richiesta che soddisfa l'offerta .
**/

int search_in_offer(char *copia_req, unsigned int select, int i);

/**
Calcola gli accoppiamenti, li scrive sul file di log ed li invia al rispettivo client (colui che ha effettuato la richiesta).

\param user_req  puntatore alla richiesta;
\param i  indice massimo utilizzato dagli array "offerte_utilizzate" e "stringhe_utilizzate";
\param fd_log  file descriptor del log file;

\retval 0  se è andato tutto a buon fine;
\retval -1  in caso di errore;
**/

int write_on_log_and_send(req_queue *rq, int i);

#endif

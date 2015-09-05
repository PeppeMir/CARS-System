/** \file mgcars.c
       \author Giuseppe Miraglia
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  */

#include "fun_server.h"

static pthread_cond_t cond = PTHREAD_COND_INITIALIZER;		/* condition variable utilizzata per forzare il calcolo degli accoppiamenti (thread match) */
static pthread_mutex_t mutex_sync = PTHREAD_MUTEX_INITIALIZER;	/* mutex per la gestione della sincronizzazione (thread match) */
static pthread_mutex_t mutex_tid = PTHREAD_MUTEX_INITIALIZER;	/* mutex per l'accesso esclusivo alla lista dei tid worker */

/*******************************************************************************************************************************************/
/*****************************************************		GESTORI		************************************************************/

/** 
Cleanup Handler.
**/

static void cleanup1(void *arg){

free(arg);

}

/**
Classifica, stampando, lo stato di terminazione di un thread.
**/

static void classify(int status){

switch ( status ){

case GENERICERR : { fprintf(stderr, "**DELETED**\n\n");
		    break; }

case ZERO: { fprintf(stderr, "**NORMAL**\n\n");		
	     break; }

case NOLOGIN: { fprintf(stderr, "**NO_LOGIN**\n\n");		
	     break; }

case MEXEXIT: { fprintf(stderr, "**MEXEXIT**\n\n");		
	     break; }

case MEMEXIT: { fprintf(stderr, "**MEMEXIT**\n\n");		
	     break; }

default: { fprintf(stderr, "UNKNOWN: error code %d\n",status);
	   break; }

		}	/* close switch */
}

/**
Cerca nella lista di tid, il tid corrispondente al thread in esecuzione e setta lo stato di questo come "non attivo" (vedi struttura tid_list).

\retval ZERO se tutto è andato a buon fine;
\retval GENERICERR  se il tid non è presente all'interno della lista.
**/

static int set_nonActive(){

pthread_t tid;
tid_list *pun;
int test;

tid = pthread_self();			/* ricavo il tid del thread in esecuzione */

pthread_mutex_lock(&mutex_tid);		/* accedo a tidw in modo mutuamente esclusivo */

for ( pun = tidw; pun != NULL; pun = pun->next )
		if ( pthread_equal(tid, pun->tid) != ZERO ){
						pun->active = FALSE;		/* lo setto come non attivo */
						test = TRUE;
						break; }

pthread_mutex_unlock(&mutex_tid);

if ( test == FALSE ){
	fprintf(stderr, "Tid non trovato!\n");
	return GENERICERR; }

return ZERO;
}

/** 
Gestisce la ricezione di SIGUSR1 settanto il flag di attesa indexM( --fun_server.h") al valore massimo.
**/

static void gestoreUsr(int signum){

if ( signum == SIGUSR1 )
	fprintf(stderr, "\n\n		*** SERVER: Ricevuto SIGUSR1 ***\n\n");

pthread_mutex_lock(&mutex_sync);

indexM = SEC_TIMER;

pthread_mutex_unlock(&mutex_sync);
}

/*******************************************************************************************************************************************/
/** Gestisce la ricezione di SIGTERM o di SIGINT :

- fa terminare (inviando SIGUSR2 sintetico) e raccoglie lo stato di terminazione di tutti i thread worker coinvolti nell'esecuzione ;
- forza il calcolo degli accoppiamenti per l'ultima volta e fa terminare il thread match;
- ripulisce l'ambiente deallocando la memoria relativa alle strutture dati allocate, ai file aperti e ai socket presenti in ./tmp ;
- infine, fa terminare il processo.

**/

static void gestoreTerm(int signum){

int status;
sigset_t set;
tid_list *pun;

sigemptyset(&set);				/* blocco i segnali durante l'esecuzione del gestore */	
sigaddset(&set,SIGINT);
sigaddset(&set,SIGTERM);
sigaddset(&set,SIGUSR1);

sigprocmask(SIG_SETMASK, &set, NULL);		

if ( signum != FALSE )
	fprintf(stderr, "\n\n		*** SERVER:  Ricevuto segnale di terminazione ***\n\n");
	else
	fprintf(stderr, "\n\n		*** SERVER: Creating Thread Worker Failed or no heap-memory are available, rerun the server ***\n\n");

/*** Workers ***/

pthread_mutex_lock(&mutex_tid);

for ( pun = tidw; pun != NULL; pun = pun->next )		/* cancella tutti i worker ancora "attivi" */
		if ( pun->active == TRUE )
			if ( pthread_cancel(pun->tid) != ZERO )
				fprintf(stderr, "\nErrore durante la cancellazione del thread %d\n", (int) pun->tid);

pthread_mutex_unlock(&mutex_tid);

/** terminati tutti i worker, l'accesso in mutua esclusione a tidw è garantito **/

for ( pun = tidw; pun != NULL; pun = pun->next ){		/* raccolgo il loro stato di terminazione */

		if ( pthread_join(pun->tid, (void *) &status) != ZERO )
				fprintf(stderr, "\nJoin error: worker %d\n",(int) pun->tid);

				else {
				fprintf(stderr, "Worker %d terminato con status ", (int) pun->tid);
				classify(status); }

			}

/*** Match ***/

pthread_mutex_lock(&mutex_sync);

if ( indexM < SEC_TIMER )			/* se necessario, interrompe l'attesa dei SEC_TIMER secondi */
	indexM = SEC_TIMER;

fprintf(stderr, "\n\n		*** Attesa elaborazione finale del match... ***\n\n");

pthread_cond_wait(&cond, &mutex_sync);		/* wait sulla condizione sino a signal da parte del thread match (fine elaborazione) */

if ( pthread_cancel(tidm) != ZERO )		/* cancellazione del thread match */
			fprintf(stderr, "Errore durante la cancellazione del thread Match\n");

pthread_mutex_unlock(&mutex_sync);			
	

if ( pthread_join(tidm, (void *) &status) != ZERO )			/* raccolta dello stato terminazione del match */
		fprintf(stderr, "Error join: thread Match\n");

		else {
		fprintf(stderr, "Thread Match terminato con status ");
		classify(status); } 

svuota_liste();				/* free della memoria relativa alle strutture dati allocate (liste) */

svuota_tid_list();				

free_graph(&graph);			/* deallocazione il grafo */

ERRM1 ( closeSocket(fd_sk) )		/* chiusura del socket del server */
		fprintf(stderr, "Errore nella chiusura del socket\n");

IF_CLOSE(fd1)				/* chiusura del file di log e dei due file utilizzati per la creazione della mappa */
IF_CLOSE(fd2)
IF_CLOSE(fd_log)

unlink(SERVER_SCK);			/* rimozione socket ./tmp/cars.sck */

fprintf(stderr,"\n\n\n		***	  SERVER TERMINATO	***\n\n\n");

_exit(ZERO);
}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/

static void print_msg(message_t *msg, char *username){

unsigned int i;

if ( msg->type == MSG_EXIT ){
		fprintf(stderr,"\n\nThread WORKER per il Client \"%s\" : Ricevuto MSG_EXIT\n\n", username);
		return; }

fprintf(stderr, "\n\nThread WORKER per il Client \"%s\" : Ricevuto messaggio: << %c%d", username, msg->type, msg->length);

switch ( msg->type ){

case MSG_CONNECT: {	for (i = ZERO; i < msg->length - UNO; i++)
					fprintf(stderr, "%c",*((msg->buffer)+i));
			fprintf(stderr, " >>\n\n");
			break; }

default: { fprintf(stderr, "%s\n\n",msg->buffer);
	   break; }

	}
}		

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/
/** Inizializza alcune variabili globali.

\param pun  puntatore al puntatore pun del main.
**/

static void initialize(tid_list **pun){

lista_clienti = NULL;
offerte = NULL;
richieste = NULL;
tidw->next = NULL;
tidw->active = TRUE;
*pun = tidw;

}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/
/** Thread Worker (uno per ogni client):

- riceve il messaggio di Login, lo esamina e decide se il client può inviare richieste/offerte al server;
- salva in usernm il nome del client come scritto nel msg di login;
- invia l'esito del Login al client destinatario;
- riceve le richieste/offerte vere e proprie e le analizza: decide se accodarle(send MSG_OK) o meno(send MSG_NO);

**/

void * worker( void *fd_clnt ){

message_t *msg = NULL;
unsigned int n_req = ZERO, retval;
sigset_t set;
char *usernm;

sigemptyset(&set);				

sigaddset(&set, SIGINT);		
sigaddset(&set, SIGTERM);
sigaddset(&set, SIGUSR1);

pthread_sigmask(SIG_SETMASK, &set, NULL);		/* maschero SIGINT, SIGUSR1 e SIGTERM */

ERRMALLOC( (usernm = malloc(sizeof(char) * LUSERNAME)) )			/* conterrà l'username del client servito */
				EXIT_THR(MEMEXIT) }

pthread_cleanup_push(cleanup1, usernm);			/* registrazione di cleanup handler per la pulizia della memoria in uscita */


ERRMALLOC ( (msg = malloc(sizeof(message_t))) )
				EXIT_THR(MEMEXIT) }

pthread_cleanup_push(cleanup1, msg);

ERRM1 ( receiveMessage( (int) fd_clnt, msg ) )				/* ricezione del messaggio di Login */
		EXITMEX_TH("\nWorker's Receving message")


if ( effettua_login(msg->buffer, &usernm, (int) fd_clnt) != FALSE ){			/* autenticazione fallita */ 

			fprintf(stderr, "\n	---> Thread WORKER per il Client \"%s\" : Password errata, login non riuscito <---\n\n", usernm);
			
			msg->buffer = NULL;		/* settando il puntatore a NULL, lo distinguo dal rifiuto di accodamento in SwitchSend */

			ERRM1( SwitchSend( (int) fd_clnt, msg, MSG_NO) )			/* sending MSG_NO */
						EXITMEX_TH("Worker's Sending message")
	
			ERRM1( set_nonActive() )							
				fprintf(stderr, "\nThread Worker %d: set to \"non active\" failure\n\n", (int) pthread_self()); 

			EXIT_THR(NOLOGIN) } 	/* ovviamente in caso di login scorretto non ha senso continuare */
						
			else {		/* autenticazione riuscita e usernm riempito con l'username del client */
			
			fprintf(stderr, "\n	---> Thread WORKER per il Client \"%s\" : login riuscito <---\n\n", usernm);			
			
			ERRM1( SwitchSend( (int) fd_clnt, msg, MSG_OK) ) 		/* sending MSG_OK */
						EXITMEX_TH("Worker's Sending message")
			
				}

/*  login riuscito: inizia la ricezione delle richieste/offerte di sharing */

do {	

ERRM1 ( receiveMessage( (int) fd_clnt, msg ) ){
			perror("Worker's Receiving message");

			free(msg->buffer);

			ERRM1( set_nonActive() )							
				fprintf(stderr, "\nThread Worker %d: set to \"non active\" failure\n\n", (int) pthread_self());	

			EXIT_THR(MEXEXIT) }

print_msg(msg, usernm);			/* stampa su stdout del messaggio ricevuto */

if ( msg->type != MSG_EXIT ) {

	/* controllo la correttezza del contenuto del msg e, se corretta, accodo la richiesta/l'offerta alla rispettiva lista */
												
	ERRM1( (retval = check_and_queues(msg->buffer, usernm)) ){		/* scorretta: send MSG_NO */
										
			ERRM1( SwitchSend( (int) fd_clnt, msg, MSG_NO) )  		
						EXITMEX_TH("Worker's Sending message")

						}									
			
			else {					/* corretta: send MSG_OK e accodo */

			if ( retval == MEMEXIT )		/* errori di allocazione nelle liste: strutture dati non affidabili */
					gestoreTerm(FALSE);

			ERRM1( SwitchSend( (int) fd_clnt, msg, MSG_OK) ) 		
						EXITMEX_TH("Worker's Sending message") 

			if ( retval == FALSE )
					n_req++;

					} 

				}
			
	else {	 	/* ELSE (msg->type == MSG_EXIT) */
								
	ERRM1( SwitchSend( (int) fd_clnt, msg, MSG_OK) ) 	/* invio conferma di interruzione richieste/offerte */		
				 EXITMEX_TH("Worker's Sending message")
	
	break;		/* termina il loop di acquisizione richieste/offerte */
	
			}
 
} while ( TRUE );


fprintf(stderr,"\n\n	=== Thread WORKER per il client %s : chiusura fase di accodamento richieste/offerte ===\n\n", usernm);

pthread_cleanup_pop(TRUE);		/* disinstallazione ed esecuzione della handler di cleanup (ordine inverso alla registrazione): free(msg) */

if ( n_req == ZERO )		/* nessuna richiesta, il client può terminare (nessun motivo di rimanere connesso) */
	ERRM1( SwitchSend( (int) fd_clnt, msg, MSG_SHAREND) ) 			
				 EXITMEX_TH("Worker's Sending message")
		
pthread_cleanup_pop(TRUE);		/* disinstallazione ed esecuzione della handler di cleanup: free(username) */

ERRM1( set_nonActive() )		/* setto il thread come "non attivo" */
	fprintf(stderr, "\nThread Worker %d: set to \"non active\" failure\n\n", (int) pthread_self()); 

return (void *) ZERO;
}



/**********************************************************************************************************************************/
/**********************************************************************************************************************************/
/**
Thread match:

1) viene attivato quando:
	- sono passati SEC_TIMER sec;
	- si riceve un segnale di tipo SIGUSR1;
	- si termina il server (SIGTERM/SIGINT);

2) elabora la richieste pendenti e calcola gli accoppiamenti;
**/

void * match(void *arg){

sigset_t set;
double *dist;
req_queue *rq , *rq_prec = NULL;
char *rotta_req, *copia_req;
unsigned int *pred;
message_t *msg = NULL;
int i, res, dim_array, fd_app;

sigemptyset(&set);				/* mascheramento SIGINT, SIGUSR1 e SIGTERM */	
	
sigaddset(&set, SIGINT);		
sigaddset(&set, SIGTERM);
sigaddset(&set, SIGUSR1);

pthread_sigmask(SIG_SETMASK, &set, NULL);	/* registrazione modifiche sulla maschera */

indexM = ZERO;		

do {

	while ( TRUE ){		/** attesa timeout / ricezione SIGUSR1 per il calcolo degli accoppiamenti **/

		pthread_mutex_lock(&mutex_sync);	/* sincronizzazione */

		if ( indexM < SEC_TIMER )
				indexM++;

				else {
				pthread_mutex_unlock(&mutex_sync); 
				break; }

		pthread_mutex_unlock(&mutex_sync);

		sleep(UNO);  }

	/** Inizio dell'elaborazione **/

	pthread_mutex_lock(&mutex_list);

	rq = richieste;

	if ( (res = piazza_rotte(offerte)) != FALSE ){		/* per ogni offerta, calcolo e sistemazione delle rotte nel rispettivo campo della struttura  */
			if ( res == GENERICERR )
				fprintf(stderr, "Funzione piazza rotte: rotta inesistente!\n"); 
				else 
				perror("Piazza rotte");

			pthread_mutex_unlock(&mutex_list);
	
			EXIT_THR(GENERICERR) }

	/*** Inizio scansione della lista delle richieste ***/

	while ( rq && offerte ){	

		if ( !(dist = dijkstra(graph, rq->start, &pred)) ){		/* calcolo dei cammini minimi dalla città di partenza della richiesta */
					perror("Dijkstra");
					EXIT_THR(GENERICERR) }

		free(dist);

		/* creazione della stringa contenente la rotta minima della richiesta selezionata */

		if ( !(rotta_req = shpath_to_string(graph, rq->start, rq->end, pred)) ){
	
							if ( errno == ZERO )
								fprintf(stderr,"Rotta inesistente!\n");
								else
								perror("Calcolo rotta");

							EXIT_THR(GENERICERR) }


		ERRMALLOC( (copia_req = malloc(sizeof(char) * (strlen(rotta_req) + UNO) )) )		/* copia della rotta della richiesta in esame */
								free(rotta_req);
								EXIT_THR(MEMEXIT) }			

		strcpy(copia_req, rotta_req);
		rotta_richiesta = rotta_req;		/* inzializzazione puntatore ausiliario: punta all'inizio della rotta della richiesta */

		/* stima del numero max di offerte utili a soddisfare la richiesta puntata da rotta_req e allocazione di offerte_utilizzate  */

		dim_array = conta_max_offerte(rotta_req);	

		if ( (i = search_in_offer(copia_req, ZERO, ZERO)) != GENERICERR ){		/* se esiste almeno un offerta che soddisfa la richiesta */
				
			ERRM1 ( (fd_app = write_on_log_and_send(rq, i)) )	/* calcolo degli accoppiamenti, scrittura sul logfile e invio di MSG_SHARE */
					pthread_exit( (void *) GENERICERR); 

			ERRM1 ( (res = sistema_code(&rq, &rq_prec, i)) )	/* rimozione della richiesta e modifica della lista delle offerte */
					pthread_exit( (void *) GENERICERR); 

					else 
					if ( res == UNO ){	/* esaurite le offerte relative al client */

						ERRM1( SwitchSend( fd_app, msg, MSG_SHAREND) )			/* invio di SHAREND al rispettivo client */
								EXITMEX_TH("Sending MSG_SHAREND")

						}	
					}		/* close then primo if */ 

				else {		/* nessun offerta ( o insieme di queste ) soddisfa la richiesta */

				rq_prec = rq;
				rq = rq->next; }

		/** free della memoria relativa all'iterazione corrente **/

		free(offerte_utilizzate);

		for ( res = ZERO ; res < dim_array; res++)	
			if ( stringhe_utilizzate[res] )
				free(stringhe_utilizzate[res]);

		free(stringhe_utilizzate);
		free(pred);

		free(rotta_req);

				}	/* close while */

	pthread_mutex_unlock(&mutex_list);	/* sblocco delle liste */

	/** fine elaborazione **/

	pthread_mutex_lock(&mutex_sync);	/* sincronizzazione */

	indexM = ZERO;			/* preparazione della nuova attesa nel ciclo di pre-elaborazione */

	pthread_cond_signal(&cond);	/* segnalazione di fine elaborazione sulla condition variable */

	pthread_mutex_unlock(&mutex_sync);

	} while ( TRUE );		


EXIT_THR(ZERO)
}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/

int main(int argc, char **argv){

int fd_sk_client, error;		/* file descriptor del socket del server e del socket del client */
struct sigaction sig;
unsigned int tid_size = ZERO;		/* dimensione della lista dei tid worker */
tid_list *pun;				/* puntatore ausiliario per attuare modifiche alla lista dei tid worker */

if ( argc != 3 )
	EXIT_2("\nErrore sul numero di argomenti.\n\n")

if ( !(fd1 = fopen( *(argv + UNO), "r")) ){		/* apertura del "file nodi" in sola lettura */
			perror("Opening file1"); 
			exit(errno); }

if ( !(fd2 = fopen( *(argv + DUE), "r")) ){		/* apertura del "file archi" in sola lettura */
			perror("Opening file2");		
			fclose(fd1);
			exit(errno); }

if ( !(graph = load_graph(fd1,fd2)) ){		/* file aperti correttamente: creazione grafo */
		perror("Loading graph");
		CLOSE_FD  }

fprintf(stdout,"\n\n	*** SERVER: Creazione Mappa riuscita ***\n\n");


if ( !(fd_log = fopen(LOGFILE, "w")) ){				/* creazione file di LOG in "."  */
			perror("Creating Logfile");
			CLOSE_FD  }
		
ERRM1 ( (fd_sk = createServerChannel(SERVER_SCK)) ){		/* creazione file socket sul quale si accetteranno le connessioni */
			perror("Creating server socket");
			CLOSE_FD }
		else
		fprintf(stderr, "\n\n	*** SERVER: in attesa di connessioni...\n\n");

bzero(&sig, sizeof(sig));		/* inizializzazione struttura */

sig.sa_handler = SIG_IGN;					/* ignoro SIGPIPE */
ERRM1 ( sigaction(SIGPIPE, &sig, NULL) )		
			EXIT_1("Registrazione SIGPIPE")

sig.sa_handler = gestoreTerm;					/* gestione di SIGTERM e SIGINT */
ERRM1 ( sigaction(SIGTERM, &sig, NULL) )		
			EXIT_1("Registrazione SIGTERM")

sigaddset(&sig.sa_mask, SIGTERM);				/* bloccata l'esecuzione dello stesso gestore durante la registrazione dello stesso successivam. */

ERRM1 ( sigaction(SIGINT, &sig, NULL) )		
			EXIT_1("Registrazione SIGINT")

sig.sa_handler = gestoreUsr;
ERRM1 ( sigaction(SIGUSR1, &sig, NULL) )			/* gestione di SIGUSR1 */
			EXIT_1("Registrazione SIGUSR1")

/**** connessione ****/

ERRMALLOC( (tidw = malloc(sizeof(tid_list))) )		/* allocazione array di tid worker */
				fclose(fd_log);
			   	closeSocket(fd_sk); 
		    		unlink(SERVER_SCK); 			
				CLOSE_FD }

initialize(&pun);		/* inizializzazione di variabili principali */

if ( (error = pthread_create(&tidm, NULL, &match, NULL)) != ZERO ){		/*** creazione thread match ***/

					fprintf(stderr, "Creating Thread Match: error %d", error);
					free(tidw);	
					closeSocket(fd_sk); 
		    			unlink(SERVER_SCK); 			
					CLOSE_FD }				


while ( TRUE ){		/* ciclo di accettazione conessioni */

	if ( (fd_sk_client = acceptConnection(fd_sk)) > ZERO ){		/* in attesa di connessione con i client: bloccante */

			pthread_mutex_lock(&mutex_tid);

			if ( tid_size > ZERO )		
				ERRMALLOC( (pun = add_thread()) )		/* alloca lo spazio per contenere le info relative al nuovo thread */
						pthread_mutex_unlock(&mutex_tid);
						gestoreTerm(FALSE); }
	
			/* creazione del thread worker: delegato a lavorare sul socket avente fd = fd_sk_client */

			if ( (error = pthread_create( &(pun->tid), NULL, &worker, (void *) fd_sk_client )) != FALSE ){
												pthread_mutex_unlock(&mutex_tid);
												gestoreTerm(FALSE); }
			tid_size++;

			pthread_mutex_unlock(&mutex_tid);
							}

	}	/* close while */


return ZERO;
}

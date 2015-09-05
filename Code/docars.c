/** \file docars.c
       \author Giuseppe Miraglia
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  */

#include "fun_client.h"

/**
Cleanup handler.
**/

static void cleanup(void *arg){

free(arg);

}

/** 
Classifica lo stato di terminazione del chiamante stampandolo su stderr.
**/

static void classify(int num, int n){

if ( n == ZERO )
	fprintf(stderr, "\nCLIENT \"%s\" :: Thread request terminato con status ",username);
	else
	if ( n == NEG )
		fprintf(stderr, "\nCLIENT \"%s\" :: Processo terminato con status ",username);
		else
		fprintf(stderr, "\nCLIENT \"%s\" :: Thread receive terminato con status ",username);	

switch (num) {

case NEG: { 	fprintf(stderr, "DELETED\n\n");
		break; }

case SHAREND_EXIT: { fprintf(stderr, "SHAREND_EXIT\n\n");
		     break; }

case MEXEXIT: { fprintf(stderr, "MEXEXIT\n\n");
		break; }

case MEMEXIT: { fprintf(stderr, "MEMEXIT\n\n");
		break; }

default:      { fprintf(stderr, "NORMAL\n\n");
		break; }

	} /* close switch */	

return;
}

/** Gestore dei segnali:
Interruzione del Processo, in seguito a ricezione di SIGINT o SIGTERM.	
**/

static void gestoreTerm(int signum){

int pid;
sigset_t set;

sigemptyset(&set);				/* blocco i segnali durante l'esecuzione del gestore */	
sigaddset(&set,SIGINT);
sigaddset(&set,SIGTERM);

sigprocmask(SIG_SETMASK, &set, NULL);

pid = getpid();

if ( signum != false )
	fprintf(stderr, "\n\n	*** Client \"%s\" \\ pid %d : ricevuto segnale d'interruzione ***\n\n",username, pid);

RM_SCK		/* chiusura e rimozione del socket */

if ( tid_req != NEG )
	if ( pthread_cancel(tid_req) != ZERO )
				fprintf(stderr, "\nCancel per il Thread request fallita!\n");

if ( signum != ZERO && tid_rx != NEG )
	if ( pthread_cancel(tid_rx) != ZERO )
				fprintf(stderr, "\nCancel per il Thread receive fallita!\n");

}

/***********************************************************************************************************************************************/
/***********************************************************************************************************************************************/

static void print_message(message_t *msg){

unsigned int i;

if ( msg->type == MSG_OK )
		return;

fprintf(stderr,"\nCLIENT \"%s\": Ricevuto ", username);

switch ( msg->type ){	/* tipi possibili di msg ricevuto */
            
case MSG_NO : { fprintf(stderr,"MSG_NO -- "); 
		break; }          

case MSG_SHARE : { fprintf(stderr,"MSG_SHARE -- "); 
		   break;}

case MSG_SHAREND : { fprintf(stderr,"MSG_SHAREND -- "); 
		   break;}

	}	/* close switch */

for ( i = ZERO; i < msg->length; i++ )
		fprintf(stderr,"%c",*((msg->buffer)+i)); 

fprintf(stderr,"\n\n");
}


/***********************************************************************************************************************************************/
/***********************************************************************************************************************************************/
/*** INPUT THREAD: 

- parsing delle richieste/offerte da stdin;
- per ogni richiesta inserita correttamente, il thread la invia al server tramite il socket creato;
- la fase interattiva termina all'inserimento di CTRL-D od EOF.

**/

void * input (void * arg){	

sigset_t set;			/* maschera */
char rd[MAXRD], type, *esito;
unsigned int i = ZERO;
message_t *msg;

sigemptyset(&set);				/* blocco i segnali SIGTERM e SIGINT relativamente al singolo thread */
sigaddset(&set, SIGINT);
sigaddset(&set, SIGTERM);
pthread_sigmask(SIG_SETMASK, &set, NULL);

ERRMALLOC( (msg = malloc(sizeof(message_t))) ){
				perror("Input Thread Malloc");
				EXIT_TH(MEMEXIT) }		

pthread_cleanup_push(cleanup, msg);

while( (esito = fgets(rd,MAXRD - OK,stdin )) ){		/* inizia la fase di prelievo di richieste/offerte da input */
		
		if ( i == ZERO )	/* salto un ciclo di fgets per skippare letture inconsistenti */
			i++;

		else {	

		switch ( type = parse(rd) ){		/* verifica la correttezza della richiesta ed elimina '/n' dalla stringa letta */

		case 'e' : { fprintf(stderr,"\nErrore di sintassi..\nPer informazioni digitare il comando %%HELP\n\n");		/* ERROR */
			     fprintf(stderr,"CLIENT \"%s\" - Thread interattivo: Attesa richieste/offerte:\n\n", username);			     
			     break; }		

		case 'h' : { printHelp();				/* %HELP */ 
			     break; }	

		case 's' : { ERRM1( send_rd(rd, msg, type) ){		/* %EXIT ==> send MSG_EXIT */
					perror("Sending MSG_EXIT");
					EXIT_TH(MEXEXIT) }	 	
			     break; }

		case 'p' : { ERRM1( send_rd(rd, msg, type) ){		/* richiesta ==> %R label1:label2 */
					perror("Sending MSG_REQUEST");
					EXIT_TH(MEXEXIT) }	
			     break; }		
				
		case 'n' : { ERRM1( send_rd(rd, msg, type) ){		/* offerta ==> label1:label2:n_posti */
					perror("Sending MSG_OFFER");
					EXIT_TH(MEXEXIT) }			     
			     break; }				
				
			}	/* end switch */

		if ( type == 's' )	/* in caso di %EXIT (il break precedente mi fa uscire solo dallo switch) */			
			break; 
			}

		}	/* END WHILE */

if ( !esito )
	ERRM1( send_rd(rd, msg, 's') ){		/* gestione caso EOF inviando MSG_EXIT: in tal caso esce dal while senza entrare nello switch */
		perror("Sending MSG_EXIT");
		EXIT_TH(MEXEXIT) }

fprintf(stderr,"CLIENT \"%s\" - Thread Interattivo: Fine inserimento richieste.\n\n", username);	

pthread_cleanup_pop(OK);

pause();

EXIT_TH(ZERO)
}


/***********************************************************************************************************************************************/
/***********************************************************************************************************************************************/
/* RECEVING THREAD:

- ascolta i messaggi inviati dal server tramite il socket dedicato;
*/

void * receving (void * arg){

int rd;
sigset_t set;
message_t *msg;

sigemptyset(&set);		/* blocco dei segnali SIGTERM e SIGINT */
sigaddset(&set, SIGINT);
sigaddset(&set, SIGTERM);
pthread_sigmask(SIG_SETMASK, &set, NULL);

ERRMALLOC( (msg = malloc(sizeof(message_t))) ){
				perror("Thread Receive Malloc");
				EXIT_TH(MEMEXIT) }

pthread_cleanup_push(cleanup, msg);

do {
 
ERRM1 ( (rd = receiveMessage(fd_sock,msg)) ){
			fprintf(stderr, "\nCLIENT \"%s\" :: ",username);
			perror("Receiving message");
			gestoreTerm(ZERO);
			EXIT_TH(MEXEXIT) } 

if ( msg->type == MSG_SHAREND ){
			fprintf(stderr, "\nCLIENT \"%s\":: End of Sharing.\n\n", username);
			fflush(stderr);
			gestoreTerm(ZERO);
			EXIT_TH(SHAREND_EXIT) }

print_message(msg);

if ( msg->type != MSG_OK )
		free(msg->buffer);

} while ( true );		/* fino a segnalazione di fine delle richieste pendenti */

pthread_cleanup_pop(true);


EXIT_TH(ZERO);
}


/***********************************************************************************************************************************************/
/***********************************************************************************************************************************************/

int main(int argc, char **argv){

int error1,error2, status;
struct sigaction sig;

if ( argc != 2 )			/* check numero argomenti */
	EXIT_2("Errore sul numero di argomenti;\nPer info invocare il comando 'INFO' quindi riprovare.\n\n")
	
if ( strlen( *(argv+1) ) > LUSERNAME )
		EXIT_2("Username troppo lungo, riprovare.\n\n")
		else
		username = *(argv + UNO);

bzero(&sig, sizeof(sig));				/* inizializzazione struttura */

sig.sa_handler = gestoreTerm;				/* gestione di SIGINT e SIGTERM */
ERRM1( sigaction(SIGINT, &sig, NULL) )
			EXIT_1("Recording signal handler")

sigaddset(&sig.sa_mask, SIGINT);

ERRM1( sigaction(SIGTERM, &sig, NULL) )
			EXIT_1("Recording signal handler")

sig.sa_handler = SIG_IGN;				/* ignoro SIGPIPE */
ERRM1( sigaction(SIGPIPE, &sig, NULL) )
			EXIT_1("Recording signal handler")

tid_req = tid_rx = NEG;

fprintf(stderr,"Insert password:  ");			/** richiesta password utente **/
scanf("%s",psw);	

ERRM1 ( (fd_sock = openConnection(SERVER_SCK)) )	/** tentativo di connessione al socket del server **/
			EXIT_1("Opening connection")

fprintf(stderr,"\n\n	***	CLIENT \"%s\": Connessione con il socket del server stabilita	***\n\n", username);

ERRMALLOC( (msgProc = malloc(sizeof(message_t))) ){
				perror("Malloc");
				RM_SCK
				classify(MEMEXIT, NEG);
				exit(MEMEXIT); }

pthread_cleanup_push(cleanup, msgProc);

ERRM1( sendLogin( getpid() ,msgProc ) )			/** invio del messaggio di LOGIN per stipulare la connessione:: << username\0psw\0sck_clientpid >> **/
		EXIT_1("CLIENT: Sending MSG_CONNECT")

ERRM1 ( receiveMessage(fd_sock, msgProc) )		/* e attesa della risposta del server: MSG_OK || MSG_NO (connessione accettata || rifiutata ) */
		EXIT_1("CLIENT: Receive message")

if ( msgProc->type == MSG_NO ){		
		fprintf(stderr, "\n	=== CLIENT \"%s\" : Login rifiutato:: terminazione ===\n\n", username);
		free(msgProc->buffer);
		free(msgProc);
		_exit(NEG); }	

	else
	if ( msgProc->type == MSG_OK )	
		fprintf(stderr,"\n	=== CLIENT \"%s\": Login riuscito!! ===\n\n", username);


	/*** autenticazione riuscita (MSG_OK) ==> posso inviare richieste al server: creazione dei due thread  ***/

if ( (error1 = pthread_create( &tid_req, NULL, &input, NULL )) != ZERO || (error2 = pthread_create( &tid_rx, NULL, &receving, NULL )) != ZERO ) {
						fprintf(stderr,"Creating threads; error numbers %d vs %d\n",error1,error2);
						free(msgProc);
						RM_SCK
						_exit(MEMEXIT); }

if ( pthread_join(tid_req, (void *) &status) != ZERO )					
				fprintf(stderr, "\nJoin per il Thread request fallita!\n");
				 	
classify(status, ZERO);

if ( pthread_join(tid_rx, (void *) &status) != ZERO )
				fprintf(stderr, "\nJoin per il Thread receive fallita!\n");
				
classify(status, OK);	

pthread_cleanup_pop(true);

RM_SCK			/** pulizia dell ambiente in caso di terminazione senza ricezione di segnali **/

fprintf(stderr, "\n		*** CLIENT \"%s\" Terminato! ***\n\n", username);

_exit(ZERO);
}

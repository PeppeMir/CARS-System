/** \file fun_server.c
       \author Giuseppe Miraglia
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  */

#include "fun_server.h"

static pthread_mutex_t mutex_cl = PTHREAD_MUTEX_INITIALIZER;	/* semaforo per la gestione della lista dei Client connessi */

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/

static void cleanup(void *arg){

free(arg);

}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/

void svuota_liste(){

offer_queue *aux1;
req_queue *aux2;
archive *aux3;

while ( offerte ){
	aux1 = offerte;
	offerte =offerte->next;
	free(aux1->rotta);
	if ( aux1->rotta_str )
		free(aux1->rotta_str);
	free(aux1); }

while ( richieste ){
	aux2 = richieste;
	richieste = richieste->next;
	free(aux2); }

while ( lista_clienti ){
	aux3 = lista_clienti;
	lista_clienti = lista_clienti->next;
	free(aux3->username);
	free(aux3->password);
	free(aux3); }
	
}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/

void svuota_tid_list(){

tid_list *pun = tidw;

while (  pun != NULL ){
		tidw = tidw->next;
		free(pun); 
		pun = tidw; }

}


/**********************************************************************************************************************************/
/**********************************************************************************************************************************/

tid_list * add_thread(void){

tid_list *pun;

for ( pun = tidw ; pun->next != NULL ; pun = pun->next );	/* spostamento in coda */

ERRMALLOC( (pun->next = malloc(sizeof(tid_list))) )
				return NULL; }

pun->next->active = TRUE;
pun->next->next = NULL;

return pun->next;
}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/

int piazza_rotte(offer_queue *pun){

unsigned int *pred;
double *dist;

while ( pun ){

	if ( !(pun->rotta_str) ){	/* potrebbe essere stata "piazzata" da una precedente invocazione del match, se l'offerta non è stata sfruttata */

		if ( !(dist = dijkstra(graph, pun->rotta->start, &pred)) )
								return errno;	

		free(dist);
		
		if ( !(pun->rotta_str = shpath_to_string(graph, pun->rotta->start, pun->rotta->end, pred)) ){
									if ( errno == ZERO )
										STD_EXIT
										else
										return errno; }
		free(pred);

		 		}

	pun = pun->next; 							
			}	/* close while */



return ZERO;
}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/


int effettua_login(char *buffer, char **usernm, int fd){

char user[LUSERNAME], pwd[LUSERNAME];
unsigned int len_u, len_p;		/* lunghezze di username e password, relativamente */
char *save_buffer = buffer;
archive *aux;

pthread_cleanup_push(cleanup, buffer);

len_u = strlen(buffer) + UNO;		/* calcolo username e relativa lunghezza */
strncpy(user, buffer, len_u);
buffer += len_u;
	
len_p = strlen(buffer) + UNO;		/* calcolo password e relativa lunghezza  */
strncpy(pwd, buffer, len_p);

strcpy(*usernm, user);		/* il buffer del thread worker(puntato da usernm) viene riempito con l'username contenuto nella stringa */

pthread_cleanup_pop(ZERO);

pthread_mutex_lock(&mutex_cl);	/* mutua esclusione: blocco agli altri worker l'accesso alle liste */

if ( !lista_clienti ){		/* lista vuota */

	ALLOCA(lista_clienti)
	
	lista_clienti->next = NULL; 	

	strcpy(lista_clienti->username, user);
	strcpy(lista_clienti->password, pwd);
	lista_clienti->fd_c = fd;

	pthread_mutex_unlock(&mutex_cl);	/* sblocco liste */

	free(save_buffer);

	return ZERO; }
		
	else {	/* se c'è almeno un cliente, controllo tra quelli se vi è già ( se si è connesso in precedenza almeno una volta) */

	for ( aux = lista_clienti; aux != NULL; aux = aux->next ){	/* HP: non sono concessi due client con username uguali */			 
							
			if ( strcmp(aux->username, user) == ZERO ){
						if ( strcmp(aux->password, pwd) == ZERO ){	/*** autenticazione riuscita ***/

										aux->fd_c = fd;			/* aggiorno il file descriptor */
										pthread_mutex_unlock(&mutex_cl);	/* sblocco liste */
										free(save_buffer);
										return ZERO; }

								else {		/*  ***password errata*** -- non coincidente col precedente accesso ***/

								pthread_mutex_unlock(&mutex_cl);	/* sblocco liste */
								free(save_buffer);
								STD_EXIT } }

			if ( !aux->next )	/* sono sull'ultimo elemento della lista */		
				break;

			}	/* close for */

	/* in caso di break */
	ALLOCA(aux->next)
	strcpy(aux->next->username, user);
	strcpy(aux->next->password, pwd);
	aux->next->fd_c = fd;
	aux->next->next = NULL;

	pthread_mutex_unlock(&mutex_cl);	/* sblocco liste */
	free(save_buffer);
	return ZERO; }	  /* close else */
				
	
return ZERO;
}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/

int SwitchSend( int fd, message_t *msg, char type){

unsigned int len;
char *aux;

switch ( type ){

case MSG_OK: { msg->length = ZERO;
	       msg->buffer = NULL;
	       msg->type = MSG_OK;
	       ERRM1( sendMessage(fd, msg) ) 
				STD_EXIT
	       break; }

case MSG_NO: { 
		if ( !msg->buffer )			/* dovuto al Login */ 
			len = strlen(REFUSELOGIN) + UNO;
			else				/* dovuto a richieste/offerte ingestibili */
			len = strlen(REFUSEREQOFF) + UNO;

	      	ERRMALLOC( (aux = malloc(sizeof(char) * len )) )
					STD_EXIT }

		pthread_cleanup_push(cleanup, aux);

		msg->length = len; 
	       	msg->type = MSG_NO;
		
		if ( !msg->buffer )
	       		strcpy(aux, REFUSELOGIN); 	/* motivo del rifiuto */			
	       		else
			strcpy(aux, REFUSEREQOFF);
	       
		msg->buffer = aux;

		ERRM1( sendMessage(fd, msg) ) 
				STD_EXIT

		pthread_cleanup_pop(TRUE);

		break; }

case MSG_SHARE: { 
		msg->type = MSG_SHARE;
		  
		ERRM1( sendMessage(fd ,msg) )
				STD_EXIT

		fprintf(stderr, "\nThread MATCH: Inviata risposta << MSG_SHARE >>\n\n"); 	
		break; }

case MSG_SHAREND: { 
		ERRMALLOC( (msg = malloc(sizeof(message_t))) )
					STD_EXIT }

		pthread_cleanup_push(cleanup, msg);

		msg->type = MSG_SHAREND;
		msg->length = ZERO;
		msg->buffer = NULL; 

		ERRM1 ( sendMessage(fd, msg) )
				STD_EXIT

		fprintf(stderr, "\nThread MATCH: Inviata risposta << MSG_SHAREND >>\n\n");
		pthread_cleanup_pop(TRUE);

		break; }
	
		} 	/* close switch */

return ZERO;
}


/**********************************************************************************************************************************/
/**********************************************************************************************************************************/

int check_and_queues(char *buffer, char *usernm){

char first[LLABEL], second[LLABEL];
char *pos, *aux;
unsigned int i, c1, c2, nposti;

req_queue *help, *scorr;
offer_queue *scan;

pthread_cleanup_push(cleanup, buffer);

pos = strchr(buffer,DDOT);

for ( i = ZERO, aux = buffer; aux != pos; aux++, i++ )		/* acquisisco città_partenza */
				first[i] = *aux;
first[i] = TCHR;

ERRM1 ( (c1 = is_node(graph,first)) ){			/* check di esistenza città_partenza: retval node index */
			errno = EBADMSG;
			free(buffer);
			STD_EXIT }
aux++;

for ( i = ZERO; ( *aux != TCHR && *aux != DDOT ) ; i++, aux++ )	/* acquisisco seconda città : in caso di %R i secondi ':' non ci sono!! */
					second[i] = *aux;
second[i] = TCHR;

ERRM1 ( (c2 = is_node(graph, second)) ){ 			/* check di esistenza città_arrivo: retval node index */
			errno = EBADMSG;
			free(buffer);
			STD_EXIT }

/* check ok: le città appartengono alla mappa; pronti per accodare....in quale coda?? */

ERRMALLOC( (help = malloc(sizeof(req_queue))) )
			free(buffer);
			return MEMEXIT; }

help->start = c1;			/* inserimento città di partenza */
help->end = c2;				/* inserimento città d'arrivo */
strcpy(help->usernm, usernm);	/* inserimento username */

pthread_cleanup_pop(ZERO);

pthread_mutex_lock(&mutex_list);	/* blocco liste */

if ( *aux == DDOT ){	/* è un offerta */
		
		nposti = atoi(++aux);		/* sistemazione numero di posti */
		
		if ( offerte ){		/* lista non vuota */

			for ( scan = offerte; scan->next != NULL; scan = scan->next);	/* mi posiziono sull'ultimo elemento */

			ERRMALLOC( (scan->next = malloc(sizeof(offer_queue))) )			/* creo l'offerta */
								free(buffer);
								pthread_mutex_unlock(&mutex_list);
								return MEMEXIT; }
			scan = scan->next;					/* posizionamento sul nuovo elemento della lista */
			scan->rotta = help;					/* riempimento campo rotta con la struttura allocata precedente (req_queue) */
			scan->posti = nposti; 					/* inserimento numero di posti string -> int */
			scan->rotta_str = NULL;
			scan->next = NULL; }

		else {			/* lista vuota */

		ERRMALLOC( (offerte = malloc(sizeof(offer_queue))) )
						free(buffer);
						pthread_mutex_unlock(&mutex_list);
						return MEMEXIT; }
		offerte->next = NULL;
		offerte->rotta_str = NULL;
		offerte->rotta = help;
		offerte->posti = nposti; }

		free(buffer);

		pthread_mutex_unlock(&mutex_list);	/* sblocco liste */

		return UNO;

		}	/* close then: offerta */

	else {		/* se invece è una richiesta */

	if ( richieste ){	/* lista non vuota */
		
		for ( scorr = richieste; scorr->next != NULL; scorr = scorr->next);	/* sull'ultimo elemento */
		scorr->next = help;
		scorr->next->next = NULL;

		}
		else {
		richieste = help;
		richieste->next = NULL; }

			}	/* close else: richieste */

free(buffer);	

pthread_mutex_unlock(&mutex_list);	/* sblocco liste */

return ZERO;
}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/

int search_fd_user(char *usr){

archive *helper = lista_clienti;

while ( helper ){

	if ( strcmp(usr, helper->username) == ZERO )
					return helper->fd_c;

	helper = helper->next;
		}

STD_EXIT
}


/**********************************************************************************************************************************/
/**********************************************************************************************************************************/

int check_req( char *usernm ){

req_queue *rq;
int count;

pthread_mutex_lock(&mutex_list);

for ( count = ZERO, rq = richieste; (rq != NULL) && (count < 2) ; rq = rq->next )
			if ( strcmp(usernm, rq->usernm) == ZERO )
							count++;		
					
pthread_mutex_unlock(&mutex_list);

if ( count == ZERO )
	STD_EXIT

return count;
}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/

int sistema_code(req_queue **rq, req_queue **rq_prec, int i){

offer_queue *aux, *prec = offerte;
int j, res;

/* decremento n° richieste relative all'utente in questione */

ERRM1 ( (res = check_req( (*rq)->usernm )) )
				STD_EXIT

/* 1) elimino la richiesta dalla coda */

if ( *rq == richieste ){	/* se è in testa */		
		richieste = richieste->next;
		free(*rq);
		*rq = richieste; }

	else {
	(*rq_prec)->next = (*rq)->next;
	free(*rq);
	*rq = (*rq_prec)->next; }

/* 2) elimino/decremento le offerte: indirizzi contenuti nell'array offerte_utilizzate */

for( j = ZERO; j <= i; j++ ){		/* itero fino all'ultimo indice, compreso! */

		aux = offerte_utilizzate[j];
		(aux->posti)--;

		if ( aux->posti == ZERO ){

			if ( aux == offerte )
				offerte = offerte->next;
					 
				else {

				for ( prec = offerte; prec->next != aux; prec = prec->next );
				prec->next = aux->next;
					}

			free(aux->rotta);
			free(aux->rotta_str);
			free(aux);
			}	

	}  /* close for */

return res;
}


/**********************************************************************************************************************************/
/**********************************************************************************************************************************/

int conta_max_offerte(char *rotta_req){

unsigned int n, i;

for ( n = ZERO; ; n++ ){
	
		if ( !(rotta_req = strchr(++rotta_req, DLR)) )
						break;
			
			}

/* alloco l'array di puntatori delle offerte utilizzate per sodd la richiesta */

ERRMALLOC( (offerte_utilizzate = malloc(sizeof(offer_queue *) * n)) )
					STD_EXIT }

/* alloco l'array di puntatori alle stringhe utilizzate */
		
ERRMALLOC( (stringhe_utilizzate = malloc(sizeof(offer_queue *) * n)) )
					STD_EXIT }	

for ( i = ZERO; i < n; i++ ){					/* inizializzazione a NULL di tutti i puntatori dell'array */
		offerte_utilizzate[i] = NULL;
		stringhe_utilizzate[i] = NULL; }

return n;
}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/

void parzial_save(char *global, char *pos){

char buffer[LLABEL];

for ( ; *(pos - UNO) != DLR ; pos--);		/* sposto il puntatore sul primo carattere della stringa che si vuole copiare */

strcpy(buffer, pos);				/* copio in buffer la prima città della rotta */
			
global = strstr(global, buffer);		/* mi sistemo sulla città inziale, però della stringa originale */

if ( da_trovare_sx )
	free(da_trovare_sx);

da_trovare_sx = malloc(sizeof(char) * (strlen(global) + UNO) );

strcpy(da_trovare_sx, global); 

}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/

offer_queue * scan(char *s, offer_queue *off_pun){

if ( !off_pun )
	return NULL;
else {

if ( strstr(off_pun->rotta_str, s) )
			return off_pun;
	else 
	return scan(s, off_pun->next);
}

return NULL;
}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/

int search_in_offer(char *copia_req, unsigned int select, int i){

offer_queue *off_pun;
char *aux;

if ( select != ZERO )
		da_trovare_sx = NULL; 

if ( !(off_pun = scan(copia_req, offerte)) ){
			
			aux = copia_req + strlen(copia_req);	/* aux punta al '\0' */
			for( ; *aux != DLR; aux--);		/* "accorcio" copia_req di una città verso sx */
			*aux = TCHR;
			if ( strchr(copia_req, DLR) ){		

				parzial_save(rotta_richiesta, aux);		/* registra la porzione finale della stringa come "da trovare" */
				return search_in_offer(copia_req, ZERO, i); }

				else {			/* caso base: la situazione PISA\0LIVORNO\0LUCCA non va bene, deve esserci almeno un $ per ricorrere */
			 	free(copia_req);
				free(da_trovare_sx);
				da_trovare_sx = NULL;	
				return GENERICERR; }
		}

else {

offerte_utilizzate[i] = off_pun;	/** mantiene il riferimento all offerta che soddisfa la richiesta/sua parte nell'array di puntatori "offerte_utilizzate" **/
stringhe_utilizzate[i] = copia_req;	/** mantiene il riferimento alla stringa soddisfatta (può essere parziale) **/


if ( da_trovare_sx) 
	return search_in_offer(da_trovare_sx, UNO, i + UNO);
	else
	return i;
		}
return ZERO;
}

/**********************************************************************************************************************************/
/**********************************************************************************************************************************/

int write_on_log_and_send(req_queue *rq, int i){

offer_queue *sel;
message_t *msg;
int j, dim_usoff, dim_usreq, dim_rotta, fd;
char *helper;

fd = search_fd_user(rq->usernm);

ERRMALLOC( (msg = malloc(sizeof(message_t))) )
			STD_EXIT }

for ( j = ZERO; j <= i; j++ ){		/* fino all'indice i (passato) */

		sel = offerte_utilizzate[j];	/* offerta j-esima */

		dim_rotta = strlen(stringhe_utilizzate[j]);	/* lunghezza rotta */
		dim_usoff = strlen(sel->rotta->usernm);		/* lunghezza utente che offre */
		dim_usreq = strlen(rq->usernm);			/* lunghezza utente che richiede */

		/* +3: 2 * '$' + '\0' oppure '\n' */

		ERRMALLOC( (msg->buffer = malloc(sizeof(char) * (dim_rotta + dim_usoff + dim_usreq + 3))) )
										STD_EXIT }	
	
		strncpy(msg->buffer, sel->rotta->usernm, dim_usoff);
		helper = msg->buffer + dim_usoff;
		*helper = DLR;

		strncpy(++helper, rq->usernm, dim_usreq);
		helper = helper + dim_usreq;
		*helper = DLR;

		strncpy(++helper, stringhe_utilizzate[j], dim_rotta);
		helper = helper + dim_rotta;
		*helper = TCHR;	

		/* buffer pronto: formato user_offer$user_req$partenza$...$arrivo */

		msg->length = strlen(msg->buffer) + UNO;

		ERRM1( SwitchSend( fd, msg, MSG_SHARE) )			/* invio accoppiamento al client */
					EXITMEX_TH("Sending MSG_SHARE")

		fprintf(fd_log, "%s\n", msg->buffer);		/* scrittura accoppiamento sul log file */

		free(msg->buffer);
		
		}
fflush(fd_log);
free(msg);

return fd;
}

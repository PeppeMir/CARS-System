/** \file shortestpath.c
       \author Giuseppe Miraglia
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "shortestpath.h"

/** infinito -- valore di distanza per l'output dell'algoritmo di Dijkstra */
#define INFINITY (-1.0)
#define UNDEF ((unsigned int) -1)

/** EMPTY: valore per segnalare una posizione vuota della coda */
#define EMPTY -1
#define START 0
#define CH_DOLLAR '$'
#define TCHAR '\0'

#define MALLOC(X,Y,Z) (X) = malloc( (Y) * (Z) )
#define MALLOC_IF(X,Y,Z) if( !( (X) = malloc( (Y) * (Z) ) ) ) \
							return NULL;

/** Funzione di utilità(aggiuntiva): estrae l'elemento avente distanza minima;

\param queue puntatore alla coda
\param dim  dimensione della coda
\param dist  puntatore all'array delle distanze

\retval el_min indice dell'elemento avente distanza minore se esistono elementi in coda
\retval -1 se la coda è vuota
*/

static unsigned int extract_min(unsigned int *queue, unsigned int dim, double *dist){

unsigned int i = START, el_min, saved;
double dist_min;

for ( ; (i < dim) && (queue[i] == EMPTY) ; i++ );

if( i == dim )
	return EMPTY;

el_min = queue[i];
saved = i;
dist_min = dist[queue[i++]];

for ( ; i < dim; i++ )
	if ( (queue[i] != EMPTY) && (dist[queue[i]] < dist_min) ){
					el_min = queue[i];
					dist_min = dist[queue[i]]; 
					saved = i; }
	
queue[saved] = EMPTY;
	
return el_min;
}


/****************************/
/** Funzione di utilità (aggiuntiva): mi dice se un elemento è già presente nell array puntato da queue;

\param queue  puntatore alla coda
\param size  dimensione dell coda
\param label indice dell'elemento da ricercare

\retval TRUE se l'elemento di indice label è già in coda
\retval FALSE viceversa
*/

static bool_t ok_insert(unsigned int *queue, int size, unsigned int label){

bool_t trovato = TRUE;
unsigned int i;

for(i = START; i < size && trovato == TRUE; i++)
	if( queue[i] == label)
			trovato = FALSE;	

return trovato;
}

/****************************/

double * dijkstra (graph_t *g, unsigned int source, unsigned int **pprec){

double *dist;	/* array delle distanze */
unsigned int *queue;	/* coda con priorità */
unsigned int i, coda = START, min;	/* coda tiene traccia per il prossimo inserimento in queue; min corrisponde al minimo elemento estratto da queue */
edge_t *aux;


/* FASE 1: inizializzazione */

if ( !g || !(g->node) || source < START || source > (g->size)-1 ){
							errno = EINVAL;
							return NULL; }

MALLOC_IF(dist, sizeof(double), g->size)

if ( !(MALLOC(queue, sizeof(unsigned int), g->size)) ){
						free(dist);
						return NULL; }
 
if ( pprec )
	if( !(MALLOC( *pprec, sizeof(unsigned int), g->size)) ){
							free(dist);
							free(queue);
							return NULL; }

for ( i = START; i < g->size; i++ ){
		queue[i] = EMPTY;	/* inzializzazione della coda, dell' array delle distanze e di quello dei predecessori (se richiesto) */
		dist[i] = INFINITY;
		if ( pprec )
			(*pprec)[i] = UNDEF; 	
			}

dist[source] = START;		/* la radice dista "0" da se stessa */

queue[coda++] = source;		/* radice inserita in testa alla coda */


/* FASE 2: ricerca cammini minimi; 

usando il criterio dell'estrazione del minimo dalla coda( funzione estract_min) e del controllo che verifica se il nodo è già presente in coda (funzione 		ok_insert), è dimostrato che ogni nodo viene inserito/estratto una ed una sola volta dalla stessa	*/

while( (min = extract_min(queue, g->size, dist)) != EMPTY ){	/* estrae il nodo con distanza da source minore; viene estratto -1 solo se la coda è vuota */

		aux = ((g->node)+min)->adj;

		for(; aux != NULL; aux = aux-> next)
				if( dist[aux->label] == INFINITY || ( dist[min] + (aux->km) ) < dist[aux->label] ){
												
											dist[aux->label] = dist[min] + aux->km;
											if( pprec )
												(*pprec)[aux->label] = min;
									
											if( ok_insert(queue,g->size,aux->label) ) 
													queue[coda++] = aux->label; }
							}

free(queue);

return dist;
}

/************************/
/** Funzione di utilità (aggiuntiva): restituisce il numero di predecessori di un nodo (dipendente dal nodo da cui si sta effettuando la visita del grafo);

\param sorg indice del nodo sorgente
\param dest indice del nodo destinazione
\param pred puntatore all'array dei predecessori

\retval lun il numero di predecessori
*/

static int conta_pred(unsigned int sorg, unsigned int dest, unsigned int *pred){

unsigned int step = dest, lun;
bool_t abort = FALSE;

for(lun = START ; !abort ; lun++)
	if( (step = pred[step]) == EMPTY)
				 abort = TRUE;				

return lun;
}

/*************************/

char * shpath_to_string(graph_t *g, unsigned int n1, unsigned int n2,  unsigned int *prec){

char *rotta, *position;
int *succ;
unsigned int i, lun, actual = n2, dim_rotta = START;

if( !g || !(g->node) || !prec ){
			errno = UNDEF;
			return NULL; }


if( prec[n2] == UNDEF) {	/* se n2 è settato a -1 nell'array dei predecessori, la rotta non esiste */
		errno = START;				
		return NULL; }

lun = conta_pred(n1,n2,prec);		/* vediamo da quante città è costituita la rotta */

MALLOC_IF(succ, sizeof(int), lun)	/* conterrà ,in modo ordinato, gli indici delle città della rotta */

for(i = START; i < lun; i++){		
	dim_rotta += strlen( ((g->node)+actual)->label ) + 1;	/* dimensione esatta (in n° caratteri) della rotta, considerando anche i "$" e il '\0' finale */
		
	succ[lun -1 -i] = actual; 	/* inserisco gli indici in succ a ritroso */
		
	actual = prec[actual]; 
		}	

if( !(MALLOC(rotta, sizeof(char), dim_rotta)) ){
					free(succ);
					return NULL; }

for(i = START , position = rotta ; i < lun;  i++ , position++){
		dim_rotta = strlen( ((g->node)+ succ[i])->label );
		strncpy( position, ((g->node)+ succ[i])->label , dim_rotta);
		position += dim_rotta;
		*position = CH_DOLLAR ; }


*(position-1) = TCHAR;

free(succ);

return rotta;
}

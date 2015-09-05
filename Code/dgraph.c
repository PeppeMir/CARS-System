/** \file dgraph.c
       \author Giuseppe Miraglia
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include "dgraph.h"

#define NEG -1
#define ZERO 0
#define TCHAR '\0'
#define UNO 1
#define STD_OUT1 free(app); \
		 free_graph(&graph); \
	   	 errno = EINVAL; \
		 return NULL;

/* una stringa-arco può essere lungo al massimo PDIM1:
	LLABEL(max lunghezza nodo sorgente) + UNO(primi ':') + LLABEL(max lunghezza nodo destinazione) + UNO (secondi ':') + 
			+ LKM (max lunghezza distanze) + UNO ('\n' letto nel file) + UNO ('\0' inserito) */

#define PDIM1 (LLABEL*2)+LKM+4
#define PDIM2 LLABEL+2

#define MALLOC(X,Y,Z) (X) = malloc( (Y) * (Z) )
#define MALLOC_IF(X,Y,Z) if( !( (X) = malloc( (Y) * (Z) ) ) ) \
							return NULL;


/***********************/

/** Funzione di utilità (aggiuntiva): controlla se la stringa-nodo passata contiene SOLO caratteri alfanumerici;

\param str  puntatore all array di stringhe
\param n  numero di stringhe dell'array

\retval TRUE  se tutte le stringhe presenti sono alfanumeriche
\retval FALSE  altrimenti
*/

static bool_t check_node(char **str, unsigned int n){

int i, j, dim;
char *p;

for(i= ZERO; i < n; i++){
	dim = strlen(str[i]);
	p  = str[i]; 	
	for(j= ZERO; j < dim; j++)	
		if( p[j] != ' ' && !ispunct(p[j]) && !isalnum( p[j] ) )
						return FALSE;
			}

return TRUE;
}


/*****************/

void free_graph (graph_t **g){

edge_t *helper;
unsigned int i;

if( g  && (*g)){	

	if( (*g) -> node ){

		for(i = ZERO; i < (*g)->size; i++){	
		
			free( (((*g)->node)+i)->label );   /* libera la memoria contenente la stringa del nodo */
		
			while( (((*g)->node)+i)->adj ){	/* libera la memoria associata alle adj di ogni nodo di g */
					helper = (((*g)->node)+i)->adj;
					(((*g)->node)+i)->adj = (((*g)->node)+i)->adj->next;					
					free(helper); }
						}

		free((*g)->node);
				}
	free((*g));
	
	*g = NULL;
	}	/* closing first if... */

}

/*****************/

graph_t * new_graph (unsigned int n, char **lbl){

graph_t *grafo;
unsigned int i;

if( (n <= ZERO) || !lbl || !check_node(lbl,n)) {
					errno = EINVAL; 
		   			return NULL; }

MALLOC_IF(grafo, sizeof(graph_t), UNO) 


if( !( MALLOC(grafo->node, sizeof(node_t), n)) ) {
					free(grafo);						
					return NULL; }

grafo->size = n;				 

for(i = ZERO; i<n; i++){
	((grafo-> node)+i) -> adj = NULL;
	if( !( MALLOC( ((grafo-> node)+i)->label, sizeof(char), LLABEL+UNO ) ) ){
									free_graph(&grafo);	
									return NULL; }						 
	
	strcpy( (((grafo-> node)+i) -> label) , lbl[i]); 
		}

return grafo;	
}


/*****************/


void print_graph (graph_t *g){

unsigned int i;
edge_t *helper;

if( g ){ 

printf("\nGrafo di %d nodo/i;\n\n",g->size);

for(i = ZERO; i < (g->size); i++){
		printf("%s :: ",((g->node)+i)->label);			
		helper = ((g->node)+i)->adj;
		for( ; helper != NULL; helper = helper->next)
				printf("Adiacenza:%d  km:%.1f ; ",helper->label, helper->km);	/* km stampati fino alla prima cifra dopo la virgola */
		printf("\n\n");			
			}
	}
}


/*****************/

/** Funzione di utilità aggiuntiva: modifica la copia del grafo aggiungendole le liste di adiacenza di tutti i nodi;

	\param newg  puntatore alla copia del grafo
	\param g  puntatore al grafo originario

	\retval newg se va tutto bene
	\retval NULL se occorrono problemi riguardanti l'allocazione in memoria 
*/

static graph_t * copy_adj(graph_t *newg, graph_t *g){

edge_t *aux, *aux2, *helper;
unsigned int i;
int flag = NEG;	

for(i = ZERO; i < (g->size); i++){	/* sono sicuro che il grafo esiste(la funzione è chiamata da copy_graph)  */

		aux = ((g->node)+i)->adj;
		for( ; aux != NULL ; aux = aux->next){	/* copio per intero le adj */
				
			if( !(((newg->node)+i)->adj) ){	
					MALLOC_IF( ((newg->node)+i)->adj, sizeof(edge_t), UNO)					
					
					((newg->node)+i)->adj->label = aux->label;
					((newg->node)+i)->adj->km = aux->km;
					((newg->node)+i)->adj->next = NULL;  }
						
						else	/* adj con almeno un elemento */
					{ 
					  if(flag == NEG){ 	
						aux2 = ((newg->node)+i)->adj;
						flag = ZERO; }
		
					  MALLOC_IF(helper, sizeof(edge_t), UNO)
					  
					  helper->label = aux->label;
					  helper->km = aux->km;
					  helper->next = NULL; 
					  aux2->next = helper;
					  aux2 = helper;
					  helper = NULL; }
					}
		flag = NEG;
		}
return newg;
}


/*****************/


graph_t *  copy_graph (graph_t *g){

graph_t *newg = NULL;
char **app;	/* array di stringhe per la chiamata a new_graph */
int i, j, error = FALSE;

if( !g ){	/* se il grafo da copiare non esiste è inutile continuare */
     errno = EINVAL; 
     return NULL; }

if( !(g->node) ){
	MALLOC_IF( newg, sizeof(graph_t), UNO)
	
	newg->size = g->size;
	return newg; }

MALLOC_IF( app, sizeof(char *), g->size)

for(i = ZERO; i < (g->size); i++){		
	if( !(MALLOC( app[i], sizeof(char), LLABEL) ) ){
				for(j = ZERO; j < i; j++)
						free(app[j]);
				free(app);  
				return NULL; }
	
	strcpy( app[i], ((g->node)+i)->label );  }
						

if( (newg = new_graph(g->size,app)) ){	/* se la copia del grafo (copia dell'array di nodi) avviene con successo */
			
		if( !(newg = copy_adj(newg,g)) ){		/* faccio una copia delle liste di adiacenza di g */
						error = TRUE;	
						free_graph(&newg); }   /*  per non lasciare a metà la copia del grafo*/						
						
						}

for(i = ZERO; i < (g->size); i++)	/* app non mi serve più */
			free(app[i]);
free(app);

if(error)
	return NULL;
 	
return newg;
}


/*****************/


bool_t is_edge(graph_t* g, unsigned int n1, unsigned int n2){

edge_t *aux;
int trovato = FALSE;

if( !g || !(g->node) )
		return FALSE;

aux = ((g->node)+n1)->adj;
			
for( ;(aux != NULL) && !trovato ; aux = aux->next)
				if(aux->label == n2)
					trovato = TRUE;		
return trovato;
}

/*****************/
/** Funzione di utilità (aggiuntiva) : copia nella stringa dest tutti i caratteri che predeceno quello puntato da segna_posto

param dest  stringa in cui copiare i caratteri
param strn  stringa da cui copiare 
param segna_posto  puntatore al carattere limite

retval i+1  indice del carattere successivo a quello puntato da segna_posto
*/

static int parzial_cpy(char *dest,char *strn, char *segna_posto){

unsigned int i;

for(i = ZERO; (strn+i) != segna_posto; i++)
			*(dest+i) = *(strn+i);
*(dest+i) = TCHAR;

return (i+UNO);
}

/*****************/


int is_node(graph_t *g, char *ss){

int i, trovato = NEG;

if( !g || !ss ){
	errno = EINVAL;
	return NEG; }

if( !(g->node) )
	return NEG;

for(i = ZERO; (i < g->size) && (trovato == NEG);i++)
		if( !strcmp(ss, ((g->node)+i)->label) )
						trovato = i;	

return trovato;
}


/*****************/

/** Funzione di utilità aggiuntiva : controlla se una stringa contiene caratteri alfanumerici / solo cifre da [0-9];

\param source  puntatore alla stringa da controllare
\param limit   puntatore che delimita la fine della stringa
\param flag    flag per moderare l'esecuzione della parte alfanumerica / numerica:
	se flag == TRUE, si controlla se la stringa è alfanumerica
	se flag == FALSE, si controlla se contiene soltanto 0 <= numeri <= 9

\retval TRUE  se la stringa è alfanumerica / numerica
\retval FALSE se non lo è o se il delimitatore di stringa è un puntatore nullo

*/

static int check_edge(char *source, char *limit, int flag){

unsigned int i = ZERO;
int count = ZERO;

if( !limit )
	return ZERO;

if( flag ){
	for( ; 	(source+i) != limit; i++)
			if( source[i] != ' ' && isalnum(source[i]) == ZERO)
								return ZERO;
			}
	else
  { 
    for( ; *(source+i) != TCHAR ; i++){
			if(source[i] == '.')		/* il '.' non deve occorrere più di una volta */
					count++;
			if( count > UNO || (source[i] != '.' && isdigit(source[i]) == ZERO) )
										return ZERO;
			}
		}
return TRUE;	
}


/*****************/


int add_edge (graph_t *g, char *e){

int isaved_sorg = NEG, isaved_dest = NEG;	/* variabili locali che mantengono gli indici del nodo sorgente e del nodo destinazione del grafo g */

char *segna_posto, *app;		/* utilizzati rispettivamente per mantenere il riferimento al carattere ":" e alle
					   tre divisioni (realizzate successivamente) della stringa "e" passata alla funzione */
edge_t *aux, *helper;

if( !g  || !e ){
	errno = EINVAL;
	return NEG; }

/* ---FASE 1--- : individuo l'indice del nodo sorgente */

/* delimito il primo pezzo di stringa, fino ai primi ":" (se esistono) e lancio un check sulla correttezza sulla 1° parte di "e" */

segna_posto = strchr(e,':');	

if( !segna_posto || !check_edge(e, segna_posto, TRUE) ) {
						errno = EINVAL;
						return NEG; }

if( !( MALLOC(app, sizeof(char), LLABEL)) )
				return NEG;

parzial_cpy(app,e,segna_posto);		/* copio la prima parte di "e" (quella riguardante il nodo sorgente) in app */

if( ( isaved_sorg = is_node(g,app) ) < ZERO) {		/* se trovo una corrispondenza al nodo sorgente richiesto, ne prendo l'indice */		
				free(app);	
				errno = EINVAL;			
				return NEG; }
free(app);		
	
/* ---FASE 2---: individuo l'indice del nodo destinazione */
	
/* mi sistemo sul secondo pezzo di stringa e la delimito */

e = segna_posto + UNO;
segna_posto = strchr(e,':');

/* check sulla corretteza della 2° parte di "e" */

if( !segna_posto || !check_edge(e, segna_posto, TRUE) ){
					errno = EINVAL;
					return NEG; }

if( !( MALLOC(app, sizeof(char), LLABEL)) )
				return NEG;
	
parzial_cpy(app,e,segna_posto);		/* come per la FASE1, copio la sottostringa di "e" (riguardante il nodo destinazione) in "app" */	
	
if( ( isaved_dest = is_node(g,app) ) < ZERO){		/* individuo l'indice del nodo destinazione e ne salvo l'indice (se esiste) */
				free(app);				
				errno = EINVAL;						
				return NEG; }
free(app);


/* ---FASE 3--- : individuo la distanza in km */

/* 	mi sistemo sul terzo ed ultimo pezzo di sottostringa( quella riguardante i km) e lancio un check sulla correttezza della 3° parte 
				della stringa : deve risultare composta da soli numeri [0-9] 	*/

e = segna_posto + UNO; 	

if( !check_edge(e, segna_posto, FALSE) ){
			errno = EINVAL;
			return NEG; }


/* ora bisogna controllare se l'arco individuato da "e" esiste già */ 

if( !(((g->node)+isaved_sorg)->adj) ){

	if( !( MALLOC( ((g->node)+isaved_sorg)->adj, sizeof(edge_t), UNO)) ) 		 
								return NEG;	
	((g->node)+isaved_sorg)->adj->label = isaved_dest;
	((g->node)+isaved_sorg)->adj->next = NULL;
	sscanf(e,"%lf",&(((g->node)+isaved_sorg)->adj->km));  } 	/* legge dalla stringa "e" ,converte in double e inserisce nel campo km */
						
	else	/* adj con almeno un elemento */
	{
	  aux = ((g->node)+isaved_sorg)->adj;	
		  
	  if( (is_edge(g,isaved_sorg,isaved_dest)) ){	 /* arco già presente  nella adj?? */
					errno = EINVAL;	
					return NEG; }
			
			else	/* arco non presente nella adj: si può procedere all'inserzione */
		{
		  for(; aux -> next != NULL; aux = aux->next);	 /* mi sposto in coda: se la adj è vuota il ciclo non parte */
		  
		  if( !(MALLOC(helper, sizeof(edge_t), UNO)) )   
							return NEG;
		  aux->next = helper;
		  helper->label = isaved_dest;
		  helper->next = NULL;
		  sscanf(e,"%lf",&(helper->km)); } 	/* legge dalla stringa "e",converte in double e inserisce nel campo km */
			  	
			}	
return ZERO;
}


/*****************/


int degree(graph_t *g, char *lbl){

edge_t *aux;
int count = ZERO, trovato;

if( !g || !lbl || !(g->node) ){
			errno = EINVAL;
			return NEG; }

if( (trovato = is_node(g,lbl)) <= NEG){	
			errno= EINVAL;
			return NEG;  }

aux = ((g->node)+trovato)->adj;

for(; aux != NULL; aux = aux->next)
			   count++;

return count;
} 


/*****************/


int n_size(graph_t *g){

if( !g ){
	errno = EINVAL;
	return NEG; }

return ( ((g->size) >= ZERO)? (g->size) : NEG );
}


/*****************/

int e_size(graph_t *g){

unsigned int i;
int count = ZERO;
edge_t *aux;

if( !g || !(g->node) ){
		errno = EINVAL;
		return NEG; }

for(i = ZERO; i < g->size; i++){
	aux = ((g->node)+i)->adj;
	for(; aux != NULL; aux=aux->next)
				count++;
			}
return count;
}


/*****************/
/** Funzione di utilità aggiuntiva: rimuove il carattere di newline da una stringa sostituendolo con il carattere di terminazione;

\param source  puntatore alla stringa dalla quale rimuovere il '\n'

\retval result puntatore alla nuova stringa senza '\n'
\retval NULL  in caso di errore
*/

static char * remove_newline(char *source){

char *result;
int dim, i = ZERO;

dim = strlen(source);	/* retval dim of source (NOT including '\0' char) */


if ( !source || !(MALLOC(result, sizeof(char), dim)) )
						return NULL;

for( ; *(source+i) != '\n' && i < dim; i++)
				result[i] = source[i];

result[i] = TCHAR;	/* rimpiazzo '\n' con '\0' */
 
return result;
}

/*****************/


graph_t * load_graph (FILE * fdnodes, FILE * fdarcs){

unsigned int i;	
graph_t *graph;		/* punterà al grafo da creare */

char *app, *p, *segna_posto, *line, *flag;	/* vari puntatori ausiliari */
edge_t *aux, *helper;

/* - dim conterrà l'esatta dimensione della stringa rappresentante il nodo sorgente, 
   - i_sorg e i_dest conterranno gli indici dei nodi sorg/dest dell'arco letto  */

int conta = ZERO, dim, i_sorg, i_dest;	


if( !fdnodes || !fdarcs ){		/* file non esistenti o errore nella precedente apertura?? */
		errno = EINVAL;
		return NULL; }

				/* ---FASE 1--- : creazione nodi sorgente */

MALLOC_IF(app, sizeof(char), PDIM2)

while ( fgets(app, PDIM2 - UNO, fdnodes) )	/* per la correttezza della struttura da allocare, mi serve sapere quanti nodi leggerò */
				conta++;
free(app);

if(conta <= ZERO){
	errno = EINVAL;
	return NULL; }

MALLOC_IF(graph, sizeof(graph_t), UNO)

if( !(MALLOC(graph->node, sizeof(node_t), conta)) ){		/* allocazione array dei 'conta' nodi */
					free(graph);
					return NULL;  }

graph->size = conta;

rewind(fdnodes);

/* inserimento nodi */

for(i = ZERO; i < conta ;i++){	

		/* char * PDIM2 = (LLABEL+2) considerando anche il '\n' letto e il '\0' inserito dalla fgets */
		
		if( !(MALLOC(app, sizeof(char), PDIM2) ) ){
							free_graph(&graph);
							return NULL; }
		
		fgets( app, PDIM2, fdnodes);	/* copia in app un intera riga del file (compreso il carattere '\n' : risulterà 
							d'intralcio per la successivi confronti (es. chiamata di is_node); allora lo elimino... */ 
		dim = strlen(app);
		
		app[dim - UNO] = TCHAR;		/* e al suo posto inserisco '\0' per terminare la stringa */

		if( !check_node(&app,UNO) || !( MALLOC( ((graph->node)+i)->label, sizeof(char), dim)) ){
											free(app);
											free_graph(&graph);
											return NULL; }

		
		strncpy( ((graph->node)+i)->label, app, dim);		/* copio la stringa fino all'ex '\n' compreso (ora '\0') */

		free(app);


		((graph->node)+i)->adj = NULL; 	
		}


					/* ---FASE 2--- : creazione archi */

MALLOC_IF(app, sizeof(char), PDIM1)

while( (fgets( app, PDIM1 - UNO, fdarcs)) ){		/* start loop per la lettura delle stringhe-arco */

	/* 1° PARTE: nodo sorgente */

segna_posto = strchr(app,':');

/* check di correttezza della 1° parte della stringa letta */

if( !segna_posto || !check_edge(app, segna_posto, TRUE) ){ 
						STD_OUT1 
						}

MALLOC_IF(p, sizeof(char), LLABEL+UNO)		/* per contenere il nodo sorgente: stavolta basta +1, dato che non viene letto '\n' */

flag = app + (parzial_cpy(p, app, segna_posto));		/* copio la parte relativa al nodo sorgente e mi sistemo dopo i primi ':' */

if( (i_sorg = is_node(graph,p)) < ZERO){
				free(p);
				STD_OUT1
				 }

free(p);
			
	/* 2° PARTE: nodo destinazione */

/* mi sistemo sui secondi ':' e lancio un check sulla correttezza della 2° sottostringa (relativa al nodo destinazione) */

segna_posto = strchr(flag,':');	
		
if( !segna_posto || !check_edge(flag, segna_posto, TRUE) ) {
							STD_OUT1
							  }

MALLOC_IF(p, sizeof(char), LLABEL+UNO)

flag = flag + parzial_cpy(p,flag,segna_posto);	/* copio la seconda sottostringa e mi sistemo subito dopo i secondi ':' */


/* 3° parte: distanze in km + creazione elemento lista di adiacenza del nodo sorgente; 
 	non proseguo se il nodo destinazione non è valido, se esiste già un arco uguale a quello che vogliamo aggiungere oppure se la 3° sottostringa 
			(relativa alla distanza in km) è malformata*/

line = remove_newline(flag);


if( ((i_dest = is_node(graph,p)) < ZERO) || ((is_edge(graph,i_sorg,i_dest)) ) || !check_edge(line, segna_posto, FALSE) ){	
														free(p);
														free(line);
														STD_OUT1
															 }

free(app);
free(p);


if( !(((graph->node)+i_sorg)->adj) ){
			if( !(MALLOC( ((graph->node)+i_sorg)->adj, sizeof(edge_t), UNO) ) ){
											free_graph(&graph);
											return NULL; 	
												}
			((graph->node)+i_sorg)->adj->label = i_dest;
			((graph->node)+i_sorg)->adj->next = NULL;
			sscanf(line,"%lf",&(((graph->node)+i_sorg)->adj->km)); }
		
		else	/* adj con almeno un elemento */
	{
	  aux = ((graph->node)+i_sorg)->adj;
	  for(; aux->next != NULL; aux = aux->next);

	  if( !(MALLOC(helper, sizeof(edge_t), UNO) ) ){
					free_graph(&graph);
					return NULL; }
	  helper->label = i_dest;
	  helper->next = NULL;
	  aux->next = helper;
	  sscanf(line, "%lf", &(helper->km)); 
				}

free(line);

if( !(MALLOC(app, sizeof(char), PDIM1) ) ){
					free_graph(&graph);
					return NULL; }

		} /* close while */


free(app);

return graph;
}


/*************************/


int save_graph (FILE * fdnodes, FILE * fdarcs, graph_t *g){

int i;
edge_t *aux;

if( !fdnodes || !fdarcs || !g ){
			errno = EINVAL;
			return NEG; }

/* ---FASE 1--- : scrittura dei nodi */

for(i = ZERO;i < g->size;i++)
		fprintf(fdnodes,"%s\n",((g->node)+i)->label);

/* ---FASE 2--- : scrittura degli archi */

for(i = ZERO; i < g->size; i++){
	aux = ((g->node)+i)->adj;	
	for(; aux != NULL; aux = aux->next){
		fprintf(fdarcs,"%s:%s:",((g->node)+i)->label,((g->node)+(aux->label))->label);		
		fprintf(fdarcs,"%.1f\n",aux->km);
					}
		}
return ZERO;
}

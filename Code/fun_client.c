/** \file fun_client.c
       \author Giuseppe Miraglia
     Si dichiara che il contenuto di questo file e' in ogni sua parte opera
     originale dell' autore.  */

#include "fun_client.h"

int sendLogin(int proc_pid, message_t *msg){

char sck[CLIENTSCKLBL], *pos;
unsigned int i;

msg->type = MSG_CONNECT;

sprintf(sck,"Client_%d.sck",proc_pid);		/* creazione socket name univoco */

sprintf(sck_name, "./tmp/%s",sck);		/* e salvataggio in buffer globale */				

msg->length = strlen(psw) + strlen(username) + strlen(sck) + 3;	/* +3 per i '\0' */

ERRMALLOC( (msg->buffer = malloc(sizeof(char) * (msg->length))) )
					EXIT_3(NEG)

strcpy(msg->buffer,username);

pos = strchr(msg->buffer,TCHR);

strcpy( ++pos,psw );

pos = strchr(pos,TCHR);

strcpy( ++pos,sck );	/*** msg pronto:  << username\0psw\0sckclientpid >> ***/

ERRM1 ( sendMessage(fd_sock,msg) ){	/* lo invio al server tramite il socket creato */
			free(msg->buffer);
			EXIT_3(NEG) }

printf("CLIENT \"%s\" : Messaggio di Login inviato : << %c%d.",username, msg->type,msg->length);

for ( i = ZERO; i < msg->length - UNO; i++)
			printf("%c",msg->buffer[i]);
printf(" >>   ");

free(msg->buffer);

return ZERO;	
}

/***********************************************************************************************************************************************/
/***********************************************************************************************************************************************/

int count_lbl(char **str, char *pos) {

unsigned int count;

for ( count = ZERO; *str != pos; (*str)++, count++); 		/* dimensione di label */ 

if ( count > LLABEL ) 		/* eccesso */ 
		EXIT_3(NEG) 

EXIT_3(ZERO)
}

/***********************************************************************************************************************************************/
/***********************************************************************************************************************************************/

char parse(char *str){

char type = 'e', *pos;

pos = strchr(str, '\n');
*pos = TCHR;		/* eliminato il '\n' */

switch ( *str ){

case '%' : { if ( *(str + UNO) == 'R' ){	/* caso %R label1:label2 */
			if ( *(str+2) != ' ' )
					EXIT_3('e')
		
			str += 3;		/* mi sposto sul 1° char di label1 */
			
			if ( *str == DDOT || !(pos = strchr(str,DDOT)) ) /* error: non c'è label1 oppure non ci sono i ':' nella stringa */ 
						EXIT_3('e') 

			ERRM1 ( count_lbl(&str,pos) )	/* lunghezza label1 */
						EXIT_3('e')

			if ( *(++str) == TCHR )	/* non esiste label2 */ \
					EXIT_3('e')

			pos = strchr(str, TCHR);

			ERRM1 ( count_lbl(&str,pos) )	/* lunghezza label2 */
						EXIT_3('e')	
			
			EXIT_3('p');	}	/* end %R */
	  	
		else
		if ( *(str + UNO) == 'E' ){		/* caso %EXIT */

			if ( strcmp( ++str, "EXIT" ) != ZERO )
						EXIT_3('e')

			EXIT_3('s'); }
		
		else
		if ( *(str + UNO) == 'H' ){		/* caso %HELP */

			if ( strcmp( ++str, "HELP" ) != ZERO )
						EXIT_3('e')
			EXIT_3('h'); }
		else
		EXIT_3('e');
		}	/* close case '%' */

default: { 	/* caso "normale" : LABEL1:LABEL2:N_POSTI 	*/

	if ( *str == DDOT || !(pos = strchr(str,DDOT)) ) /* error: non c'è label1 oppure non ci sono i ':' nella stringa */ 
						EXIT_3('e') 

	ERRM1 ( count_lbl(&str,pos) )		/* lunghezza label1 */
				EXIT_3('e')

	if ( *(++str) == TCHR )		/* non esiste label2 */ \
			EXIT_3('e')

	if ( !(pos = strchr(str,DDOT)) )
					EXIT_3('e');

	ERRM1 ( count_lbl(&str,pos) )		/* lunghezza label2 */
				EXIT_3('e')

	if ( *(++str) == TCHR )
			EXIT_3('e')

	pos = strchr(str, TCHR);

	for ( ; str != pos; str++ )		/* ammessi solo numeri per N_POSTI */
			if ( !isdigit( *str ) )	/* retval 0 <=> *str è un numero */
					EXIT_3('e')
			
	EXIT_3('n')  }	/* close default */		
	
}	/* close switch */

return type;
}


/***********************************************************************************************************************************************/
/***********************************************************************************************************************************************/

void printHelp(){

fprintf(stdout,"\n1) OFFERTE DI SHARING:\n\n	Formato richieste:\n\n		citta_partenza:citta_arrivo:n_posti \\n\n\n\
	ad esempio:\n\n		PISA:LA SPEZIA:4\n\n");

fprintf(stdout,"2) RICHIESTE DI SHARING:\n\n	Formato offerte:\n\n		%%R citta_partenza:citta_arrivo \\n\n\n\
	ad esempio:\n\n		%%R PISA:LIVORNO\n\n");

fprintf(stdout,"3) TERMINAZIONE OFFERTE/RICHIESTE:\n\n	Per terminare la lista basta digitare:\n\n\
		%%EXIT\n\n");

fprintf(stdout,"CLIENT \"%s\" - Thread interattivo: Attesa richieste/offerte:\n\n", username);

fflush(stdout);
}


/***********************************************************************************************************************************************/
/***********************************************************************************************************************************************/

int send_rd(char *rd, message_t *msg, char type){

int wrt;

if ( type == 'n' )		/* caso label1:label2:n_posti */
	msg->type = MSG_OFFER;

	else	
	if ( type == 's' ){	/* caso %EXIT */
		msg->type = MSG_EXIT;
		msg->length = ZERO; }
		
		else {			/* caso %R label1:label2 */
		msg->type = MSG_REQUEST;
		rd += 3; }

if ( type != 's' ){
	msg->length = strlen(rd) + 1;
	ERRMALLOC( (msg->buffer = malloc(sizeof(char) * (msg->length))) )
							EXIT_3(NEG)

	strcpy(msg->buffer,rd); }

ERRM1 ( (wrt = sendMessage(fd_sock, msg)) ){  	/* invio parziale o nullo */
				if ( msg->buffer){
					wrt = errno;
					free(msg->buffer);
					errno = wrt; }
				EXIT_3(NEG) }
if ( type != 's' )
	free(msg->buffer);

return ZERO;
}


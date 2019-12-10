/*
* Autore: Francesco Bocchi
* Matricola: 550145
* Indirizzo e-mail: francesboc26@gmail.com
*
* Il progetto è stato realizzato in ogni sua parte da Francesco Bocchi.
*/

#ifndef SHAREDLIST_H_
#define SHARED_LIST_H_

#include <ops.h>
#include <message.h>
/**
 * @file  sharedList.h
 * @brief Contiene le funzioni che implementano il 
 *		  protocollo di comunicazione tra worker e 
 *		  listener e viceversa
 */

typedef struct richieste{
	int fd_c;//file descriptor dell'utente connesso
	op_t op;//operazione da eseguire/eseguita
	message_t msg;
	int check;//bit utilizzato nelle statisctiche
	struct richieste* next;
}request;

typedef struct coda{
	request* head;//testa della lista
	request* tail;//puntatore alla coda
	int len;//lunghezza della lista
}queue;

/**
 * @function initQueue
 * @brief Inizializza la coda
 *
 * @return ritorna la coda in caso di successo,
 *         NULL in caso di errore
 */

queue* initQueue();

/**
 * @function newHistoryInsert
 * @brief Inserisce il messaggio nella history del client.
 *		  Utilizzata in icl_history_insert(icl_hash.h)
 *
 * @param q 		coda dalla quale accedere alla history
 * @param msg 		messaggio da salvare nella history
 * @param histSize 	dimensione massima della history
 * @param check 	utilizzato per capire se il messagio è stato mai
 *        			prelevato dalla history o meno (mi permette di aggiornare
 *       			correttamente le statistiche)
 *
 * @return ritorna 1 in caso di successo,
 *         -1 in caso di errore
 */

int newHistoryInsert(queue* q, message_t msg,int histSize,int check);

/**
 * @function stampaHistory
 * @brief Stampa la history di un utente.
 *
 * @param q 	coda dalla quale accedere alla history
 *
 */

void stampaHistory(queue* q);

/**
 * @function freeHistory
 * @brief Libera la history di un utente.Utilizzata in
 * 		  icl_hash_delete,icl_hash_destroy(icl_hash.h)
 *
 * @param q 	coda dalla quale accedere alla history
 *
 */

void freeHistory(queue* q);

/**
 * @function getElem
 * @brief Salva i messaggi presenti nella history nell'array msg.
 *		  Funzione utilizzata in getHistory(icl_hash.h)
 *
 * @param q 		coda dalla quale accedere alla history
 * @param msg  		array utilizzato per salvare i messaggi
 * @param count 	conta i messaggi testuali che non erano stati ancora "letti"
 * 					(permette di aggionare le statistiche)
 * @param fileCount conta i file che non erano stati ancora "letti"
 *					(permette di aggiornare le statistiche)
 *
 * @return ritorna il nuemro di messaggi salvati se presenti
 */

int getElem(queue* q,message_t* msg,int* count,int* fileCount);

/**
 * @function newRequest
 * @brief Inserisce il descrittore del client fd nella coda q
 *
 * @param q 	coda nella quale inserire la richiesta (descrittore e operazione)
 * @param fd 	descrittore del client che ha fatto una nuova 
 *			 	richiesta da inserire nella coda
 * @param op 	contiene il tipo di risposta che il worker invia al listener.
 *			 	op non viene settata se la chiamata di funzione è effetuata dal
 *			 	listener, ma questa diventa significativa quando è il worker ad
 *			 	eseguire la funzione
 *
 *
 * @return ritorna 1 in caso di successo,
 *         -1 in caso di errore
 */

int newRequest(queue* q, int fd, op_t op);

/**
 * @function getRequest
 * @brief Preleva il primo descittore del client dalla coda
 *
 * @param q 	coda da cui prelevare il descrittore
 * @param arg   memorizza la richiesta da prelevare
 * 
 * @return ritorna 1 in caso di successo,
 *         -1 in caso di errore
 */

int getRequest(queue* q,request* arg);

/**
 * @function destroyRequest
 * @brief Libera la coda delle richieste
 *
 * @param q 	coda da liberare
 *
 * @return ritorna 1 in caso di successo,
 *         -1 in caso di errore
 */

int destroyRequest(queue* q);

/**
 * @function printRequest
 * @brief Stampa la lista delle richieste
 *
 * @param q coda da cui stampare la lista delle richieste
 *
 */

void printRequest(queue* q);

#endif
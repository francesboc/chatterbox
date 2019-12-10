/*
* Autore: Francesco Bocchi
* Matricola: 550145
* Indirizzo e-mail: francesboc26@gmail.com
*
* Il progetto è stato realizzato in ogni sua parte da Francesco Bocchi.
*/

#ifndef ONLINE_LIST_H_
#define ONLINE_LIST_H_

#include<config.h>
#include<message.h>
#include<pthread.h>
#include<errno.h>
/**
 * @file  online_list.h
 * @brief Contiene le funzioni che implementano la lista degli 
 *        utenti connessi
 */

typedef struct client_connessi{
	char buf[MAX_NAME_LENGTH];//nome utente
	int fd_c;//descittore utente
	pthread_mutex_t fdLock;//lock sul descrittore dell'utente
	struct client_connessi* next;
}online_users;

typedef struct online{
	online_users* head;//testa della lista degli utenti
	int online;//numero utenti online
	pthread_mutex_t onlineMTX;
}online_queue;

/**
 * @function initializeQueue
 * @brief Inizializza la coda e la variabile di lock.
 *
 * @return ritorna la coda allocata in caso di successo,
 *         NULL in caso di errore
 */

online_queue* initializeQueue();

/**
 * @function online_insert
 * @brief Inserisce l'elemento buf nella lista list
 *
 * @param list 		lista utenti connessi 
 * @param buf 		elemento da inserire nella lista
 * @param fd 		descrittore dell'elemento da inserire
 *
 * @return ritorna il numero di utenti online in caso di successo,
 *         -1 in caso di errore
 */

int online_insert(online_queue* list,char* buf,int fd);

/**
 * @function online_remove
 * @brief Rimuove l'elemento identificato da fd dalla lista list
 *
 * @param list lista utenti connessi 
 * @param fd descrittore dell'elemento da rimuovere dalla lista
 *
 * @return ritorna 1 in caso di successo,
 *         -1 in caso di errore
 */

int online_remove(online_queue* list,int fd);

/**
 * @function online_find
 * @brief Cerca l'elemento buf nella lista list
 *
 * @param list 	lista utenti connessi 
 * @param buf 	elemento da cercare nella lista
 *
 * @return ritorna il fd in caso di elemento trovato,
 *         -1 in caso di elemento non trovato o se
 *     	   buf non è definto
 */	

int online_find(online_queue* list,char* buf);

/**
 * @function getOnlineList
 * @brief Restituisce la lista degli utenti online salvandola in tmp
 *
 * @param list 	lista utenti connessi 
 * @param tmp  	array in cui salvare la lista
 *
 * @return ritorna il numero di utenti connessi,
 *         0 in caso di lista vuota
 */	

int getOnlineList(online_queue*list,online_users* tmp);

/**
 * @function searchAndLock
 * @brief Acquisisce la lock dell'elemento identificato da buf
 *
 * @param list 	lista utenti connessi 
 * @param buf  	nome dell'elemento di cui acquisire la lock
 *
 * @return ritorna un puntatore all'elemento,
 *         NULL in caso di errore
 */	

online_users* searchAndLock(online_queue* list,char* buf);

/**
 * @function nOnline
 * @brief Restituisce il numero di utenti online
 *
 * @param list 	la lista degli utenti connessi 
 *
 * @return ritorna il numero di utenti connessi in caso di 
 *		   successo, 0 in caso di lista vuota
 */	

int nOnline(online_queue* list);

/**
 * @function online_destroy
 * @brief Libera la lista degli utenti connessi
 *
 * @param list 	lista utenti connessi 
 *
 * @return ritorna 1 in caso di successo,
 *         -1 in caso di errore
 */

int online_destroy(online_queue* list);

/**
 * @function online_print
 * @brief Stampa la lista degli utenti connessi
 *
 * @param list 	lista utenti connessi 
 */

void online_print(online_queue* list);

/**
 * @function online_dump
 * @brief Scarica la lista degli utenti connessi
 *        nel buffer buf di msg.buf
 *
 * @param list lista utenti connessi 
 * @param msg  buffer per contenere gli utenti connessi
 *
 * @return ritorna 1 in caso di successo,
 *         -1 in caso di list non è definto
 */	

int online_dump(online_queue* list, message_data_t* msg);

#endif
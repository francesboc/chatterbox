/*
* Autore: Francesco Bocchi
* Matricola: 550145
* Indirizzo e-mail: francesboc26@gmail.com
*
* Il progetto è stato realizzato in ogni sua parte da Francesco Bocchi.
*/

#ifndef LISTENER_H_
#define LISTENER_H_

#ifndef HELP_H
#include <help.h>
#endif
#include <message.h>
/**
 * @file  listener.h
 * @brief Definisce il listener utilizzato per accettare
 *		  richieste dai client
 */

/**
 * @function aggiorna
 * @brief Funzione utilizzata per aggiornare
 *		  il fd maggiore nella select
 *
 * @param set maschera utilizzata nella select
 * @param fdmax indice del fd maggiore

 * @return il fd maggiore in caso di successo,
 *         -1 in caso di errore
 */

int aggiorna(fd_set set,int fdmax);

/**
 * @function listen
 * @brief Funzione eseguita dal listener thread
 *		  che gestisce le richieste
 *
 * @param arg non utilizzato

 * @return 0 in caso di successo,
 *         -1 in caso di errore
 */

void* listener(void *arg);

#endif

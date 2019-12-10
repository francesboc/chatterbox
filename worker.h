/*
* Autore: Francesco Bocchi
* Matricola: 550145
* Indirizzo e-mail: francesboc26@gmail.com
*
* Il progetto è stato realizzato in ogni sua parte da Francesco Bocchi.
*/

#ifndef WORKER_H_
#define WORKER_H_

#ifndef HELP_H_
#include <help.h>
#endif
#include <message.h>

/**
 * @file  worker.h
 * @brief Definisce il worker utilizzato per eseguire
 *		  le richieste dei client
 */

/**
 * @function work
 * @brief Funzione eseguita dal worker thread
 *		  che esegue le richieste dai client
 *
 * @param arg non utilizzato 

 * @return 0 in caso di successo,
 *         -1 in caso di errore
 */

void* work(void *arg);

#endif

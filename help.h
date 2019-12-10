/*
* Autore: Francesco Bocchi
* Matricola: 550145
* Indirizzo e-mail: francesboc26@gmail.com
*
* Il progetto è stato realizzato in ogni sua parte da Francesco Bocchi.
*/

/* 
 * @file  help.h
 * @brief Utilizzata per gli include e le define usate in tutto il progetto.
 * 		  Contiene anche variabili esterne utilizzate dai thread e le implementazioni
 * 		  di funzioni come la READN e la WRITEN e funzioni per gestire la tabella hash
 */

#ifndef HELP_H
#define HELP_H

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<errno.h>
#include<pthread.h>
#include<signal.h>
#include<assert.h>
#include<sys/socket.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h> //manipolazione file
#include<sys/mman.h> //gestione della memoria
#include<sys/un.h>
#include<sys/select.h>
#ifndef PARSER_H_
#include<parser.h>
#endif
#ifndef CONNECTIONS_H_
#include <connections.h>
#endif
#ifndef MEMBOX_STATS_
#include <stats.h>
#endif
#ifndef ICL_HASH_H_
#include <icl_hash.h>
#endif
#ifndef SHAREDLIST_H_
#include <sharedList.h>
#endif
#ifndef ONLINE_LIST_H_
#include <online_list.h>
#endif

#define SYSCALL(r,c,e) \
    if((r=c)==-1) { perror(e);exit(errno); }
#define CHECK(x,y,z) \
    if((x=y)==-1) { perror(z);return 0; }

#define lock pthread_mutex_lock
#define unlock pthread_mutex_unlock
#define wait pthread_cond_wait
#define signal pthread_cond_signal
#define broadcast pthread_cond_broadcast

extern volatile sig_atomic_t termina;//variabile per la terminazione dei thread
extern configuration conf;//struttura che contiene i campi del file di configurazione
extern queue* requestList;//lista utilizzata per la comunicazione tra listener-worker
extern queue* workedList;//lista utilizzara per la comunicazione tra worker-listener
extern icl_hash_t* hash;//tabella hash per tenere traccia degli utenti registrati
extern online_queue* online_c;//lista utilizzata per ricordare gli utenti connessi
extern struct statistics chattyStats;//struttura che memorizza alcune statistiche monitorate dal server
extern pthread_mutex_t statMTX;//mutex per aggiornare le statistiche
extern pthread_mutex_t requestMTX;//mutext per accedere alla lista delle richieste da lavorare
extern pthread_cond_t requestCOND;//variabile di condizione su cui si sospendono i worker quando non ci sono richieste 
extern pthread_mutex_t workedMTX;//mutext per accedere alla lista delle richieste lavorate

/*---------------------------------------------------------------------
 *Funzioni per liberare memoria
 */
void free_key(void* key);

void free_data(void* data);
/* --------------------------------------------------------------------- */

int readn(long fd, void *buf, size_t size);

int writen(long fd, void *buf, size_t size);

#endif
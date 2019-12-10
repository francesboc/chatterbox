/*
* Autore: Francesco Bocchi
* Matricola: 550145
* Indirizzo e-mail: francesboc26@gmail.com
*
* Il progetto è stato realizzato in ogni sua parte da Francesco Bocchi.
*/

/**
 * @file config.h
 * @brief File contenente alcune define con valori massimi utilizzabili
 */

#if !defined(CONFIG_H_)
#define CONFIG_H_

#define lock pthread_mutex_lock
#define unlock pthread_mutex_unlock
#define wait pthread_cond_wait
#define broadcast pthread_cond_broadcast
#define MAX_NAME_LENGTH 32
#define ICL_ENTRY 8192//numero di entry nell'icl hash
#define ICL_PARTITION 128//numero di lock utilizzate nell'icl hash

/* aggiungere altre define qui */



// to avoid warnings like "ISO C forbids an empty translation unit"
typedef int make_iso_compilers_happy;

#endif /* CONFIG_H_ */

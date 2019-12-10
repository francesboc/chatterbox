/*
* Autore: Francesco Bocchi
* Matricola: 550145
* Indirizzo e-mail: francesboc26@gmail.com
*
* Il progetto è stato realizzato in ogni sua parte da Francesco Bocchi.
*/

#ifndef PARSER_H_
#define PARSER_H_

#define MAXLINE 1024
#define MAXDIM 512

/**
 * @file  parser.h
 * @brief Definisce la struttura utilizzata nel parsing
 *		  Contiene inoltre le funzioni utilizzate
 */

/**
 * struttura che memorizza i dati del file
 * di configurazione.
 */
typedef struct configuration_file{
	char UnixPath[MAXLINE];
	int MaxConnections;
	int ThreadsInPool;
	int MaxMsgSize;
	int MaxFileSize;
	int MaxHistMsgs;
	char DirName[MAXLINE];
	char StatFileName[MAXLINE];
}configuration;

/**
 * @function check_row
 * @brief	Controlla se nella riga passata come parametro vi sono
 *			dati che devono essere parsati
 *
 * @param conf la struttura che contiene i campi presenti nel file di configurazione
 * @param buf  la riga passata in analisi
 *
 */

void check_row(configuration* conf,char* buf);

/**
 * @function parser
 * @brief	Effettua il parsing del file di configurazione
 *
 * @param conf la struttura che contiene i campi presenti nel file di configurazione
 * @param argv contiene il percorso del file di configurazione
 *
 */

void parser(configuration* conf,char* argv);

#endif
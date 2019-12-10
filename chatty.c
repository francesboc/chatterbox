/*
* Autore: Francesco Bocchi
* Matricola: 550145
* Indirizzo e-mail: francesboc26@gmail.com
*
* Il progetto è stato realizzato in ogni sua parte da Francesco Bocchi.
*/

/**
 * @file chatty.c
 * @brief File principale del server chatterbox
 */
#define _POSIX_C_SOURCE 200809L
#include <help.h>
#include <message.h>
#include <listener.h>
#include <worker.h>

#define FREE(x) \
	if(x != NULL) free(x);

/* 
 * struttura che memorizza le statistiche del server, struct statistics 
 * e' definita in stats.h.
 */
struct statistics  chattyStats = { 0,0,0,0,0,0,0 };

static sigset_t sigpipe;
static sigset_t sigset;
volatile sig_atomic_t termina = 0;//variabile utilizzata per la terminazione dei thread
configuration conf;//struttura che contiene i campti del file di configurazione

icl_hash_t* hash = NULL;//Tabella hash per tenere traccia degli utenti registrati
online_queue* online_c=NULL;//Lista per tenere traccia degli utenti connessi
queue* requestList=NULL;//Lista per gestire le richieste Listener-Worker
queue* workedList=NULL;//Lista per gestire il lavoro tra worker-listener

pthread_mutex_t statMTX = PTHREAD_MUTEX_INITIALIZER;//mutex per aggiornare le statistiche
pthread_mutex_t requestMTX = PTHREAD_MUTEX_INITIALIZER;//mutex per requestList
pthread_cond_t requestCOND = PTHREAD_COND_INITIALIZER;//variabile di condizione per requestList
pthread_mutex_t workedMTX = PTHREAD_MUTEX_INITIALIZER;//mutex per workedList

//thread dei segnali
void* handler(void *arg);

static void usage(const char *progname) {
    fprintf(stderr, "Il server va lanciato con il seguente comando:\n");
    fprintf(stderr, "  %s -f conffile\n", progname);
}

void* handler(void *arg){
    int x;
    FILE* ipf;
    if((ipf = fopen(conf.StatFileName,"w"))==NULL){
    	perror("fopen");
    	return (void*) 0;
    }
    while(!termina){
        if(sigwait(&sigset, &x)!=0){//aspetto il segnale
                perror("sigwait");
                errno=0;
                exit(0);
        }
        if(x==SIGUSR1){
            if(printStats(ipf)==-1)
                perror("printStats");
        }
        else termina=1;
    }
    fclose(ipf);
    return (void*) 0;
}

void cleanup(char* str){
	unlink(str);
}

int main(int argc, char *argv[]){
	if(argc < 3){
		fprintf(stderr, "ERRORE: too few arguments\n");
		usage("./chatty");
		return -1;
	}
	parser(&conf,argv[2]);
	cleanup(conf.UnixPath);

	//GESTIONE SEGNALI
	int err=0;
	sigemptyset(sigpipe);
	sigemptyset(sigset);
	
	CHECK(err,sigaddset(&sigpipe, SIGPIPE),"sigpipe");//maschero sigpipe
    CHECK(err,pthread_sigmask(SIG_BLOCK,&sigpipe,NULL),"sigmask1");//installo la maschera sigpipe.
    CHECK(err,sigaddset(&sigset, SIGINT),"sigint");//setta a 1 bit relativo a sigint
    CHECK(err,sigaddset(&sigset, SIGQUIT),"sigquit");
    CHECK(err,sigaddset(&sigset, SIGTERM),"sigterm");
    CHECK(err,sigaddset(&sigset, SIGUSR1),"sigusr1");
    CHECK(err,sigaddset(&sigset, SIGTSTP),"sigusr1");
    CHECK(err,pthread_sigmask(SIG_BLOCK,&sigset,NULL),"sigmask2");//installo in or sigset

	//INIZIO LAVORO SERVER
	int x = mkdir(conf.DirName,0777);
	if(x < 0 && errno!=EEXIST){
		perror("mkdir");
		return 0;
	}

	online_c=initializeQueue();
	requestList=initQueue();
	workedList=initQueue();

	//inizializzo tabella hash per mantenere gli utenti registrati
	hash = icl_hash_create(ICL_ENTRY,NULL,NULL);

	//pool di thread worker
	pthread_t worker[conf.ThreadsInPool];
	//listener thread
	pthread_t listenTH,sigHandler;

	if((err=pthread_create(&sigHandler,NULL,&handler,NULL))!=0){
		perror("ERRORE: creazione thread segnali");
		return -1;
	}

	if((err=pthread_create(&listenTH,NULL,&listener,NULL))!=0){
		perror("ERRORE: creazione listener");
		return -1;
	}
	
	//creazione pool di thread worker
	for(int i=0;i<conf.ThreadsInPool;i++){
		if((err=pthread_create(&worker[i],NULL,&work,NULL))!=0){
			perror("ERRORE: creazione listener");
			return -1;
		}
	}
	
	//join dei thread worker
	for(int i=0;i<conf.ThreadsInPool;i++){
		if(pthread_join(worker[i],NULL)!=0){
			perror("errore join server");
			return -1;
		}
	}

	//join del listener
	if(pthread_join(listenTH,NULL)!=0){
		perror("errore join server");
		return -1;
	}

	//join del thread dei segnali
	if(pthread_join(sigHandler,NULL)!=0){
		perror("errore join server");
		return -1;
	}
	
	destroyRequest(requestList);
	destroyRequest(workedList);
	FREE(requestList);
	FREE(workedList);
	
	icl_hash_destroy(hash, free_key, free_data);
	hash = NULL;
	online_destroy(online_c);
	FREE(online_c);

   	cleanup(conf.UnixPath);
   	return 0;
}

/*
* Autore: Francesco Bocchi
* Matricola: 550145
* Indirizzo e-mail: francesboc26@gmail.com
*
* Il progetto è stato realizzato in ogni sua parte da Francesco Bocchi.
*/

#include <listener.h>

int aggiorna(fd_set set,int fdmax){
	for(int i = (fdmax-1);i>=0;--i)
		if(FD_ISSET(i,&set)) return i;
	return -1;
}

void* listener(void *arg){

	int listenfd,err;
	SYSCALL(listenfd,socket(AF_UNIX,SOCK_STREAM,0),"creazione socket");
	struct sockaddr_un sa;
	memset(&sa,'0',sizeof(sa));
	strncpy(sa.sun_path,conf.UnixPath,UNIX_PATH_MAX);
	sa.sun_family = AF_UNIX;

	SYSCALL(err,bind(listenfd,(struct sockaddr*)&sa,sizeof(sa)),"chiamata bind");
	SYSCALL(err,listen(listenfd,SOMAXCONN),"chiamata listen");

	//Maschere da utilizzare per la select.
	fd_set set,tmpset;
	//azzero le maschere
	FD_ZERO(&set);
	FD_ZERO(&tmpset);

	//aggiungo il listenfd alla maschera set
	FD_SET(listenfd,&set);

	//fdmax è l'indice del fd con id maggiore
	int fdmax = listenfd;

	//utilizzato nella select
	request tmp;
	//timeout per non bloccarsi sulla select
	struct timeval t;

	while(!termina){
		int err;
		t.tv_sec =0;
		t.tv_usec = 100;
		//come prima cosa verifico la presenza di 'descrittori lavorati'
		lock(&workedMTX);
		while(workedList->head != NULL){
			getRequest(workedList,&tmp);
			//il client è terminato
			if(tmp.op == OP_TERM){//OP_TERM è un comando interno definito in ops.h
				close(tmp.fd_c);
			}
			else{//rimetto l'fd del client nel set
				FD_SET(tmp.fd_c,&set);
				if(tmp.fd_c > fdmax) fdmax = tmp.fd_c;
			}
		}
		unlock(&workedMTX);

		tmpset = set;
		err = select(fdmax+1,&tmpset,NULL,NULL,&t);
		if(err < 0){
			perror("select error");
			return (void*) 0;
		}

		//capiamo da quale fd ci è arrivata la richiesta
		for(int i=0; i <= fdmax; i++){
			if(FD_ISSET(i,&tmpset)){
				long fd_c;
				if(i == listenfd){
					//nuova connessione di un client
					SYSCALL(fd_c,accept(listenfd,NULL,0),"chiamata accept");
					//aggiungiamo il fd del nuovo client alla maschera set
					FD_SET(fd_c,&set);
					//teniamo aggiornato l'indice di fd maggiore
					if(fd_c > fdmax) fdmax = fd_c;
				}
				else fd_c= i; //richiesta ricevuta da client già esistente

				//Inseriamo il descrittore del client nella lista, in modo tale che un worker lo lavori
				lock(&requestMTX);
				if(newRequest(requestList,fd_c,-1)==-1){
					perror("ERRORE: newRequest");
					close(listenfd);
					unlock(&requestMTX);
					return (void*)0;
				}
				FD_CLR(fd_c,&set);
				signal(&requestCOND);
				unlock(&requestMTX);
				if(fd_c == fdmax) fdmax = aggiorna(set,fdmax);
			}
		}
	}

    //se arrivo a questo punto mi arrivato un segnale. Risveglio gli altri thread per 'avvisarli'
	lock(&requestMTX);
	broadcast(&requestCOND);
	unlock(&requestMTX);

	close(listenfd);
	return (void*)0;
}
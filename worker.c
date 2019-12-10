/*
* Autore: Francesco Bocchi
* Matricola: 550145
* Indirizzo e-mail: francesboc26@gmail.com
*
* Il progetto è stato realizzato in ogni sua parte da Francesco Bocchi.
*/

#include <worker.h>

#define CONNCHECK(x,y) \
	if(x== -1 && errno != EPIPE){ perror(y); errno=0;}

#define FREE(x) \
	if(x != NULL) free(x);

//funzione per aggiornare le statistiche
void addToStat(unsigned long* stat, int qnt, int check){
	lock(&statMTX);
	if(check)
		*stat+=qnt;
	else
		*stat-=qnt;
	unlock(&statMTX);
}

void* work(void* arg){
	int fd_c;
	int r,res;//utilizzate per vari controlli
	request tmp;

	//leggo le richieste
	message_t MSG;
	setHeader(&MSG.hdr,-1,"");
	setData(&MSG.data,"",NULL,0);

	//rispondo alle richieste
	message_t reply_c;
	setHeader(&reply_c.hdr,-1,"");
	setData(&reply_c.data,"",NULL,0);

	//utilizzato nella POSTFILE_OP
	message_data_t fileMSG;
	setData(&fileMSG,"",NULL,0);

	//utilizzate per le lock sui descrittori
	online_users* userLock;//lock sul descrittore del receiver
	online_users* replyLock;//lock sul descrittore del sender

	while(!termina){
		lock(&requestMTX);
		while(requestList->head == NULL && !termina){
			wait(&requestCOND,&requestMTX);
		}
		//controllo se il listener ha inserito qualche richiesta da lavorare
		if(getRequest(requestList,&tmp)==-1){
			unlock(&requestMTX);
			//c'è stato un errore
			return (void*)-1;
		}
		fd_c = tmp.fd_c;

		unlock(&requestMTX);

		r =readMsg(fd_c,&MSG);
		//ora conosciamo il client che ha fatto richiesta
		CONNCHECK(r,"readMSG iniziale");

		//la read ha letto qualcosa
		if(r > 0){
			switch(MSG.hdr.op){
				case REGISTER_OP:{
					//controllo che il nick non sia già registrato
					if(icl_hash_insert(hash,MSG.hdr.sender,NULL)!=NULL){//utente non registrato
						reply_c.hdr.op = OP_OK;
						//inserisco l'utente tra i connessi,dato che si è appena registrato
						if((reply_c.data.hdr.len=online_insert(online_c,MSG.hdr.sender,fd_c))==-1){
							fprintf(stderr, "ERRORE: online_insert (REGISTER_OP)\n");
							reply_c.hdr.op = OP_FAIL;
						}
						reply_c.data.hdr.len *= (MAX_NAME_LENGTH+1);
						//preparo la lista degli utenti online
						if((online_dump(online_c,&reply_c.data))==-1){
							fprintf(stderr, "ERRORE: online_dump (REGISTER_OP)\n");
							reply_c.hdr.op = OP_FAIL;
						}
						//mando la risposta OP_OK o OP_FAIL
						replyLock=searchAndLock(online_c,MSG.hdr.sender);
						res=sendHeader(fd_c,&reply_c.hdr);
						CONNCHECK(res,"sendHeader(REGISTER_OP)");

						//mando la lista degli utenti connessi
						if(reply_c.hdr.op == OP_OK){
							res = sendData(fd_c,&reply_c.data);
							CONNCHECK(res,"sendData(REGISTER_OP)");
						}
						unlock(&(replyLock->fdLock));
						replyLock=NULL;
						FREE(reply_c.data.buf);
						reply_c.data.buf=NULL;
						//aggiorno le statistiche
						lock(&statMTX);
						chattyStats.nusers++;
						chattyStats.nonline++;
						unlock(&statMTX);
					}
					else{//nick già presente
						reply_c.hdr.op = OP_NICK_ALREADY;
						replyLock=searchAndLock(online_c,MSG.hdr.sender);
						res=sendHeader(fd_c,&reply_c.hdr);
						if(replyLock!= NULL){
							unlock(&(replyLock->fdLock));
							replyLock=NULL;
						}
						CONNCHECK(res,"sendHeader(1)(REGISTER_OP)");
						
						//aggiorno le statistiche
						addToStat(&chattyStats.nerrors,1,1);
					}
				}break;
				case CONNECT_OP:{
					lock(&(online_c->onlineMTX));
					if(nOnline(online_c) < conf.MaxConnections){//posso ancora gestire utenti che si connettono
						if(icl_hash_find(hash,MSG.hdr.sender)!=NULL){//l'utente è già registrato
							if((replyLock=searchAndLock(online_c,MSG.hdr.sender))==NULL){//controllo che non sia già connesso
								reply_c.hdr.op = OP_OK;
								//inserisco l'utente nella lista degli utenti
								if((reply_c.data.hdr.len=online_insert(online_c,MSG.hdr.sender,fd_c))==-1){
									fprintf(stderr, "ERRORE: online_insert (CONNECT_OP)\n");
									reply_c.hdr.op = OP_FAIL;
								}
								//preparo la lista degli utenti online
								reply_c.data.hdr.len *= (MAX_NAME_LENGTH+1);
								if((online_dump(online_c,&reply_c.data))==-1){
									fprintf(stderr, "ERRORE: online_dump (CONNECT_OP)\n");
									reply_c.hdr.op = OP_FAIL;
								}
								//mando la risposta OP_OK o OP_FAIL
								replyLock=searchAndLock(online_c,MSG.hdr.sender);
								res=sendHeader(fd_c,&reply_c.hdr);
								CONNCHECK(res,"sendHeader(CONNECT_OP)");
								//mando la lista degli utenti connessi
								if(reply_c.hdr.op == OP_OK){
									res = sendData(fd_c,&reply_c.data);
									CONNCHECK(res,"sendData(CONNECT_OP)");
								}
								unlock(&(replyLock->fdLock));
								replyLock=NULL;
								FREE(reply_c.data.buf);
								reply_c.data.buf=NULL;
								//aggiorno le statistiche
								if(reply_c.hdr.op == OP_OK) addToStat(&chattyStats.nonline,1,1);
							}
							else{//utente già connesso
								reply_c.hdr.op = OP_NICK_ALREADY;
								res=sendHeader(fd_c,&reply_c.hdr);
								unlock(&(replyLock->fdLock));
								replyLock=NULL;
								CONNCHECK(res,"sendHeader(1)(CONNECT_OP)");
								addToStat(&chattyStats.nerrors,1,1);
							}
						}
						else{//utente non registrato e richiede di connettersi
							reply_c.hdr.op = OP_NICK_UNKNOWN;
							res=sendHeader(fd_c,&reply_c.hdr);
							CONNCHECK(res,"sendHeader(2)(CONNECT_OP)");
							addToStat(&chattyStats.nerrors,1,1);
						}
					}
					else{//il numero di connessioni è maggiore di MaxConnections
						reply_c.hdr.op = OP_TOOCONNECTIONS;
						res=sendHeader(fd_c,&reply_c.hdr);
						CONNCHECK(res,"sendHeader(3)(CONNECT_OP)");
						addToStat(&chattyStats.nerrors,1,1);
					}
				}break;
				case POSTTXT_OP:{
					if(MSG.data.hdr.len < conf.MaxMsgSize){//la dimensione del messaggio può essere gestita
						if(icl_hash_find(hash,MSG.hdr.sender)!=NULL){//il sender è registrato
							if((userLock=searchAndLock(online_c,MSG.data.hdr.receiver))==NULL){//il destinatario non è connesso
								if(icl_hash_find(hash,MSG.data.hdr.receiver)!=NULL){//il destinatario è registrato
									//salvo il messaggio nella history del destinatario
									setHeader(&reply_c.hdr,TXT_MESSAGE,MSG.hdr.sender);
									strncpy(reply_c.data.hdr.receiver,MSG.data.hdr.receiver,MAX_NAME_LENGTH);
									reply_c.data.buf = (char*) calloc(MSG.data.hdr.len,sizeof(char));
									strncpy(reply_c.data.buf,MSG.data.buf,MSG.data.hdr.len);
									reply_c.data.hdr.len = MSG.data.hdr.len;

									if(icl_history_insert(hash,MSG.data.hdr.receiver,reply_c,conf.MaxHistMsgs,1)==-1){
										fprintf(stderr, "ERRORE: icl_history_insert (POSTTXT_OP)\n");
										setHeader(&reply_c.hdr,OP_FAIL,MSG.hdr.sender);
									}
									//invio la risposta al sender che ha inviato il messaggio
									if(reply_c.hdr.op != OP_FAIL) setHeader(&reply_c.hdr,OP_OK,MSG.hdr.sender);
									replyLock=searchAndLock(online_c,MSG.hdr.sender);
									res=sendHeader(fd_c,&reply_c.hdr);
									if(replyLock != NULL)unlock(&(replyLock->fdLock));
									replyLock=NULL;
									CONNCHECK(res,"sendHeader(POSTTXT_OP)");
									FREE(MSG.data.buf);
									MSG.data.buf = NULL;
									FREE(reply_c.data.buf);
									reply_c.data.buf = NULL;
									if(reply_c.hdr.op != OP_FAIL) addToStat(&chattyStats.nnotdelivered,1,1);
									else addToStat(&chattyStats.nerrors,1,1);
								}
								else{
									//dico al sender che il destinatario a cui sta
									//cercando di inviare un messaggio non è registrato
									setHeader(&reply_c.hdr,OP_NICK_UNKNOWN,MSG.hdr.sender);
									replyLock=searchAndLock(online_c,MSG.hdr.sender);
									res=sendHeader(fd_c,&reply_c.hdr);
									if(replyLock != NULL)unlock(&(replyLock->fdLock));
									replyLock=NULL;
									CONNCHECK(res,"sendHeader(1)(POSTTXT_OP)");
									FREE(MSG.data.buf);
									MSG.data.buf = NULL;
									addToStat(&chattyStats.nerrors,1,1);
								}
							}
							else{//il destinatario è connesso. Invio il messaggio direttamente
								setHeader(&reply_c.hdr,TXT_MESSAGE,MSG.hdr.sender);
								strncpy(reply_c.data.hdr.receiver,MSG.data.hdr.receiver,MAX_NAME_LENGTH);
								reply_c.data.buf = (char*) calloc(MSG.data.hdr.len,sizeof(char));
								strncpy(reply_c.data.buf,MSG.data.buf,MSG.data.hdr.len);
								reply_c.data.hdr.len = MSG.data.hdr.len;

								//salvo il messaggio nella history
								if(icl_history_insert(hash,MSG.data.hdr.receiver,reply_c,conf.MaxHistMsgs,0)==-1){
									fprintf(stderr, "ERRORE: icl_history_insert(1)(POSTTXT_OP)\n");
									setHeader(&reply_c.hdr,OP_FAIL,MSG.hdr.sender);
								}
								res = sendRequest(userLock->fd_c,&reply_c);
								unlock(&(userLock->fdLock));
								userLock=NULL;
								FREE(reply_c.data.buf);
								reply_c.data.buf = NULL;
								CONNCHECK(res,"sendRequest(POSTTXT_OP)");
								//rispondo al mittente che l'operazione è andata a buon fine
								if(reply_c.hdr.op != OP_FAIL) setHeader(&reply_c.hdr,OP_OK,MSG.hdr.sender);
								replyLock=searchAndLock(online_c,MSG.hdr.sender);
								res=sendHeader(fd_c,&reply_c.hdr);
								if(replyLock != NULL) unlock(&(replyLock->fdLock));
								replyLock=NULL;
								CONNCHECK(res,"sendHeader(2)(POSTTXT_OP)");
								FREE(MSG.data.buf);
								MSG.data.buf = NULL;
								if(reply_c.hdr.op != OP_FAIL) addToStat(&chattyStats.ndelivered,1,1);
								else addToStat(&chattyStats.nerrors,1,1);
							}
						}
						else{//sender non registrato
							reply_c.hdr.op = OP_NICK_UNKNOWN;
							res=sendHeader(fd_c,&reply_c.hdr);
							CONNCHECK(res,"sendHeader(3)(POSTTXT_OP)");
							addToStat(&chattyStats.nerrors,1,1);
						}
					}
					else{//messaggio troppo lungo
						reply_c.hdr.op = OP_MSG_TOOLONG;
						replyLock=searchAndLock(online_c,MSG.hdr.sender);
						res=sendHeader(fd_c,&reply_c.hdr);
						if(replyLock != NULL){
							unlock(&(replyLock->fdLock));
							replyLock=NULL;
						}
						CONNCHECK(res,"sendHeader(4)(POSTTXT_OP)");
						addToStat(&chattyStats.nerrors,1,1);
					}
				}break;
				case POSTTXTALL_OP:{
					if(MSG.data.hdr.len < conf.MaxMsgSize){//controllo che il messaggio non superi la size ammessa
						if(icl_hash_find(hash,MSG.hdr.sender)!=NULL){//il sender è registrato
							//utilizzo un array per tenere occupata il meno possibile la lock onlineMTX
							online_users* tmp = (online_users*)calloc(conf.MaxConnections,sizeof(online_users));
							//acquisisco l' attuale lista degli utenti online e la savlo in tmp
							int i = getOnlineList(online_c,tmp);
							setHeader(&reply_c.hdr,TXT_MESSAGE,MSG.hdr.sender);
							reply_c.data.buf = (char*) calloc(MSG.data.hdr.len,sizeof(char));
							strncpy(reply_c.data.buf,MSG.data.buf,MSG.data.hdr.len);
							reply_c.data.hdr.len = MSG.data.hdr.len;
							//a questo punto potrei essere deschedulato e qualche client potrebbe andare offline
							//e la searchandLock successiva restituisce null. Se mi trovo in questa situazione, copio 
							//semplicemnte una stringa vuota nell'array precedentemente calcolato, in modo da settare 
							//il bit che verifica l'effettiva lettura del mex correttamente
							for(int j=0;j<i;j++){//passiamo prima il messaggio a tutti i connessi
								if(strcmp(tmp[j].buf,MSG.hdr.sender)!=0){//non mi devo inviare il messaggio da solo
									userLock = searchAndLock(online_c,tmp[j].buf);//acquisisco la lock sull'fd del receiver
									if(userLock != NULL){
										strncpy(reply_c.data.hdr.receiver,tmp[j].buf,MAX_NAME_LENGTH);
										res=sendRequest(tmp[j].fd_c,&reply_c);
										unlock(&(userLock->fdLock));
										CONNCHECK(res,"sendRequest(POSTTXTALL_OP)");
										userLock=NULL;
									}
									else{//il client non è più online.
										strcpy(tmp[j].buf,"");
									}
								}
							}
							//array sender utilizzto per capire quali utenti erano online al momento dell'invio del messaggio
							char** senderTmp = NULL;
							if(i>0){
								senderTmp =(char**) malloc(i*sizeof(char*));
								for(int j=0;j<i;j++){
									senderTmp[j]= (char*) malloc(MAX_NAME_LENGTH*sizeof(char));
									strncpy(senderTmp[j],tmp[j].buf,MAX_NAME_LENGTH);
								}
							}
							FREE(tmp);
							tmp=NULL;
							//ho inviato il messaggio a tutti gli utenti connessi,ora metto il messaggio
							//nella history di tutti gli utenti registrati
							strncpy(reply_c.data.buf,MSG.data.buf,MSG.data.hdr.len);
							if(!pushHistory(hash,reply_c,conf.MaxHistMsgs,senderTmp,i)){//l'operazione è fallita
								setHeader(&reply_c.hdr,OP_FAIL,MSG.hdr.sender);
								replyLock=searchAndLock(online_c,MSG.hdr.sender);
								res=sendHeader(fd_c,&reply_c.hdr);
								if(replyLock != NULL) unlock(&(replyLock->fdLock));
								replyLock=NULL;
								CONNCHECK(res,"sendHeader(POSTTXTALL_OP)");
								FREE(MSG.data.buf);
								MSG.data.buf = NULL;
								lock(&statMTX);
								chattyStats.ndelivered+=i;
								chattyStats.nerrors++;
								unlock(&statMTX);
							}
							else{//l'operazione è andata a buon fine
								setHeader(&reply_c.hdr,OP_OK,MSG.hdr.sender);
								replyLock=searchAndLock(online_c,MSG.hdr.sender);
								res=sendHeader(fd_c,&reply_c.hdr);
								if(replyLock != NULL) unlock(&(replyLock->fdLock));
								replyLock=NULL;
								CONNCHECK(res,"sendHeader(1)(POSTTXTALL_OP)");
								FREE(MSG.data.buf);
								MSG.data.buf = NULL;
								lock(&statMTX);
								chattyStats.nnotdelivered+=(chattyStats.nusers-i);
								chattyStats.ndelivered+=i;
								unlock(&statMTX);
							}
							FREE(reply_c.data.buf);
							reply_c.data.buf=NULL;
							if(senderTmp != NULL){
								for(int j=0;j<i;j++){
									FREE(senderTmp[j]);
								}
							}
							FREE(senderTmp);
							senderTmp=NULL;
						}
						else{//sender non registrato
							reply_c.hdr.op = OP_NICK_UNKNOWN;
							res=sendHeader(fd_c,&reply_c.hdr);
							CONNCHECK(res,"sendHeader(3)(POSTTXT_OP)");
							addToStat(&chattyStats.nerrors,1,1);
						}
					}
					else{//messaggio troppo lungo
						reply_c.hdr.op = OP_MSG_TOOLONG;
						replyLock=searchAndLock(online_c,MSG.hdr.sender);
						res=sendHeader(fd_c,&reply_c.hdr);
						if(replyLock!=NULL){
							unlock(&(replyLock->fdLock));
							replyLock=NULL;
						}
						CONNCHECK(res,"sendHeader(1)(POSTTXTALL_OP)");

						addToStat(&chattyStats.nerrors,1,1);
					}
				}break;
				case POSTFILE_OP:{
					if(icl_hash_find(hash,MSG.hdr.sender)!=NULL){//il sender è registrato
						reply_c.hdr.op = OP_OK;
						//leggo il corpo del messaggio
						int n = readData(fd_c,&fileMSG);
						CONNCHECK(n,"readData(POSTFILE_OP)");
						if(fileMSG.hdr.len < (conf.MaxFileSize*1024)){//size del file ammessa
							if(n>0){
								int ipf;
								//in fileName c'è tutto il path dove andare a salvare il file
								char* fileName = (char*)calloc(((MSG.data.hdr.len+strlen(conf.DirName)+1)),sizeof(char));
								strncpy(fileName,conf.DirName,strlen(conf.DirName));
								char* tmp = strrchr(MSG.data.buf,'/');
								if(tmp == NULL){//significa che mi è stato passato direttamente il nome del file
									strncat(fileName,"/",strlen("/"));//metto uno slash per creare il path
									strncat(fileName,MSG.data.buf,strlen(MSG.data.buf)+1);
								}
								else{
									strncat(fileName,tmp,strlen(tmp)+1);
								}
								tmp = NULL;
								errno=0;
								ipf = open(fileName, O_CREAT | O_WRONLY,0666);
								if(ipf>0){//apertura andata a buon fine
									//ora dobbiamo scriverci il contenuto
									if((n=writen(ipf,fileMSG.buf,fileMSG.hdr.len))<=0){
										perror("scrittura corpo messaggio");
										errno=0;
										reply_c.hdr.op = OP_FAIL;
									}
									if(reply_c.hdr.op == OP_OK){
										//la scrittura del corpo è andata a buon fine
										//devo inviare il messaggio. Controllo se è online o meno il destinatario
										if((userLock=searchAndLock(online_c,MSG.data.hdr.receiver))==NULL){//non è online
											//salvo il messaggio nella history
											setHeader(&reply_c.hdr,FILE_MESSAGE,MSG.hdr.sender);
											strncpy(reply_c.data.hdr.receiver,MSG.data.hdr.receiver,MAX_NAME_LENGTH);
											reply_c.data.hdr.len = MSG.data.hdr.len;
											reply_c.data.buf = (char*) calloc(MSG.data.hdr.len,sizeof(char));
											strncpy(reply_c.data.buf,MSG.data.buf,MSG.data.hdr.len);
											if(icl_history_insert(hash,MSG.data.hdr.receiver,reply_c,conf.MaxHistMsgs,1)==-1){
												fprintf(stderr, "ERRORE: icl_history_insert(POSTFILE_OP)\n");
												reply_c.hdr.op = OP_FAIL;
											}
											//invio la risposta al mittente
											if(reply_c.hdr.op != OP_FAIL) setHeader(&reply_c.hdr,OP_OK,MSG.hdr.sender);
											replyLock=searchAndLock(online_c,MSG.hdr.sender);
											res=sendHeader(fd_c,&reply_c.hdr);
											if(replyLock != NULL) unlock(&(replyLock->fdLock));
											replyLock=NULL;
											CONNCHECK(res,"sendHeader(POSTFILE_OP)");
											FREE(fileName);
											FREE(reply_c.data.buf);
											FREE(MSG.data.buf);
											fileName=NULL;
											MSG.data.buf = NULL;
											reply_c.data.buf = NULL;
											if(reply_c.hdr.op != OP_FAIL)addToStat(&chattyStats.nfilenotdelivered,1,1);
											else addToStat(&chattyStats.nerrors,1,1);
										}
										else{
											//è online
											setHeader(&reply_c.hdr,FILE_MESSAGE,MSG.hdr.sender);
											strncpy(reply_c.data.hdr.receiver,MSG.data.hdr.receiver,MAX_NAME_LENGTH);
											reply_c.data.hdr.len = MSG.data.hdr.len;
											reply_c.data.buf = (char*) calloc(MSG.data.hdr.len,sizeof(char));
											strncpy(reply_c.data.buf,MSG.data.buf,MSG.data.hdr.len);
											//salvo il messaggio nella history
											if(icl_history_insert(hash,MSG.data.hdr.receiver,reply_c,conf.MaxHistMsgs,0)==-1){
												fprintf(stderr, "ERRORE: icl_history_insert(1)(POSTFILE_OP)\n");
												reply_c.hdr.op= OP_FAIL;
											}
											res=sendRequest(userLock->fd_c,&reply_c);
											unlock(&(userLock->fdLock));
											userLock=NULL;
											CONNCHECK(res,"sendRequest(POSTFILE_OP)");
											//rispondo al mittente che l'operazione è andata a buon fine
											if(reply_c.hdr.op != OP_FAIL) setHeader(&reply_c.hdr,OP_OK,MSG.hdr.sender);
											replyLock=searchAndLock(online_c,MSG.hdr.sender);
											res=sendHeader(fd_c,&reply_c.hdr);
											if(replyLock != NULL) unlock(&(replyLock->fdLock));
											replyLock=NULL;
											CONNCHECK(res,"sendHeader(1)(POSTFILE_OP)");
											FREE(fileName);
											FREE(reply_c.data.buf);
											FREE(MSG.data.buf);
											fileName=NULL;
											MSG.data.buf = NULL;
											reply_c.data.buf = NULL;
											if(reply_c.hdr.op != OP_FAIL) addToStat(&chattyStats.nfiledelivered,1,1);
											else addToStat(&chattyStats.nerrors,1,1);
										}
									}
									else{
										//la scrittura del corpo non è andata a buon fine
										setHeader(&reply_c.hdr,OP_FAIL,MSG.hdr.sender);
										replyLock=searchAndLock(online_c,MSG.hdr.sender);
										res=sendHeader(fd_c,&reply_c.hdr);
										if(replyLock != NULL) unlock(&(replyLock->fdLock));
										replyLock=NULL;
										CONNCHECK(res,"sendHeader(2)(POSTFILE_OP)");
										FREE(fileName);
										FREE(MSG.data.buf);
										fileName=NULL;
										MSG.data.buf=NULL;
										addToStat(&chattyStats.nerrors,1,1);
									}
									close(ipf);
								}
								else{//apertura file fallita
									perror("open file");
									errno=0;
									reply_c.hdr.op = OP_FAIL;
									replyLock=searchAndLock(online_c,MSG.hdr.sender);
									res=sendHeader(fd_c,&reply_c.hdr);
									if(replyLock != NULL) unlock(&(replyLock->fdLock));
									replyLock=NULL;
									CONNCHECK(res,"sendHeader(3)(POSTFILE_OP)");
									FREE(fileName);
									FREE(MSG.data.buf);
									fileName=NULL;
									MSG.data.buf=NULL;
									addToStat(&chattyStats.nerrors,1,1);
								}
							}
							else{
								//la readdata non ha letto nulla
								reply_c.hdr.op = OP_FAIL;
								replyLock=searchAndLock(online_c,MSG.hdr.sender);
								res=sendHeader(fd_c,&reply_c.hdr);
								if(replyLock != NULL) unlock(&(replyLock->fdLock));
								replyLock=NULL;
								CONNCHECK(res,"sendHeader(4)(POSTFILE_OP)");
								FREE(MSG.data.buf);
								MSG.data.buf=NULL;
								addToStat(&chattyStats.nerrors,1,1);
							}
						}
						else{//la size del file è troppo grande
							reply_c.hdr.op = OP_MSG_TOOLONG;
							replyLock=searchAndLock(online_c,MSG.hdr.sender);
							res=sendHeader(fd_c,&reply_c.hdr);
							if(replyLock!=NULL){
								unlock(&(replyLock->fdLock));
								replyLock=NULL;
							}
							CONNCHECK(res,"sendHeader(5)(POSTFILE_OP)");
							FREE(MSG.data.buf);
							MSG.data.buf=NULL;
							addToStat(&chattyStats.nerrors,1,1);
						}
						FREE(fileMSG.buf);
						fileMSG.buf=NULL;
					}
					else{//sender non registrato
						reply_c.hdr.op = OP_NICK_UNKNOWN;
						res=sendHeader(fd_c,&reply_c.hdr);
						CONNCHECK(res,"sendHeader(3)(POSTTXT_OP)");
						FREE(MSG.data.buf);
						MSG.data.buf=NULL;
						addToStat(&chattyStats.nerrors,1,1);
					}
				}break;
				case GETPREVMSGS_OP:{
					setHeader(&reply_c.hdr,OP_OK,MSG.hdr.sender);
					if(icl_hash_find(hash,MSG.hdr.sender)!=NULL){//il sender è registrato
						message_t* tmp=(message_t*)calloc(conf.MaxHistMsgs,sizeof(message_t));
						int count=0;
						int fileCount=0;
						if(!tmp) reply_c.hdr.op = OP_FAIL;
						replyLock=searchAndLock(online_c,MSG.hdr.sender);
						res=sendHeader(fd_c,&reply_c.hdr);
						CONNCHECK(res,"sendHeader(GETPREVMSGS_OP)");
						if(reply_c.hdr.op != OP_FAIL){
							/*utilizzo un' array perchè, al costo di una piccola occupazione
							in spazio, riesco ad evitare di appesantire l'esecuzione di 
							getHistory, senza dover inviare tutti i messaggi con la partizione
							lockata */
							size_t i = getHistory(hash,MSG.hdr.sender,tmp,&count,&fileCount);
							//ho tutti i messaggi
							reply_c.data.buf = (char*)&i;
							reply_c.data.hdr.len = sizeof(size_t);
							res = sendData(fd_c,&reply_c.data);
							CONNCHECK(res,"sendData(GETPREVMSGS_OP)");
							//invio la history
							for(int j=0;j<i;j++){
								res= sendRequest(fd_c,&tmp[j]);
								CONNCHECK(res,"sendRequest(GETPREVMSGS_OP)");
							}
							if(replyLock != NULL) unlock(&(replyLock->fdLock));
							replyLock=NULL;
							for(int j=0;j<i;j++){
								FREE(tmp[j].data.buf);
							}
							FREE(tmp);
							tmp=NULL;
							reply_c.data.buf = NULL;
							lock(&statMTX);
							chattyStats.nnotdelivered-=count;
							chattyStats.nfilenotdelivered-=fileCount;
							chattyStats.ndelivered+=count;
							chattyStats.nfiledelivered+=fileCount;
							unlock(&statMTX);
						}
						else{
							if(replyLock != NULL) unlock(&(replyLock->fdLock));
							addToStat(&chattyStats.nerrors,1,1);
						}
					}
					else{
						//l'utente non è registrato
						reply_c.hdr.op = OP_NICK_UNKNOWN;
						res=sendHeader(fd_c,&reply_c.hdr);
						CONNCHECK(res,"sendHeader(1)(GETPREVMSGS_OP)");

						addToStat(&chattyStats.nerrors,1,1);
					}
				}break;
				case GETFILE_OP:{
					char* fileName = (char*)calloc(((MSG.data.hdr.len+strlen(conf.DirName)+1)),sizeof(char));
					strncpy(fileName,conf.DirName,strlen(conf.DirName));
					char* tmp = strrchr(MSG.data.buf,'/');
					if(tmp == NULL){//significa che mi è stato passato direttamente il nome del file
						strncat(fileName,"/",strlen("/"));//metto uno slash per creare il path
						strncat(fileName,MSG.data.buf,strlen(MSG.data.buf)+1);
					}
					else{
						strncat(fileName,tmp,strlen(tmp)+1);
					}
					tmp = NULL;
					//apro il file
					int fd = open(fileName, O_RDONLY);
					if (fd<0){
					   //errore nell' apertura del file
						perror("open");
						errno=0;
						setHeader(&reply_c.hdr,OP_NO_SUCH_FILE,MSG.hdr.sender);
						replyLock=searchAndLock(online_c,MSG.hdr.sender);
						res=sendHeader(fd_c,&reply_c.hdr);
						if(replyLock != NULL) unlock(&(replyLock->fdLock));
						replyLock=NULL;
						CONNCHECK(res,"sendHeader(GETFILE_OP)");
						FREE(MSG.data.buf);
						MSG.data.buf = NULL;
					}
					else{ // mappiamo il file da spedire in memoria
						char* mappedfile = mmap(NULL,conf.MaxFileSize,PROT_READ,MAP_PRIVATE,fd,0);
						if (mappedfile == MAP_FAILED) {
							perror("mmap(GETFILE_OP)");
							errno=0;
							setHeader(&reply_c.hdr,OP_NO_SUCH_FILE,MSG.hdr.sender);
							replyLock=searchAndLock(online_c,MSG.hdr.sender);
							res=sendHeader(fd_c,&reply_c.hdr);
							if(replyLock != NULL) unlock(&(replyLock->fdLock));
							replyLock=NULL;
							CONNCHECK(res,"sendHeader(1)(GETFILE_OP)");

						}
						close(fd);
						//invio op_ok al client
						setHeader(&reply_c.hdr,OP_OK,MSG.hdr.sender);
						replyLock=searchAndLock(online_c,MSG.hdr.sender);
						res=sendHeader(fd_c,&reply_c.hdr);
						CONNCHECK(res,"sendHeader(2)(GETFILE_OP)");
						FREE(MSG.data.buf);
						MSG.data.buf=NULL;
						if (mappedfile) { // devo inviare il file
							setData(&MSG.data, "", mappedfile, conf.MaxFileSize);
							if (sendData(fd_c, &MSG.data) == -1) { // invio il contenuto del file
							   	perror("sending data(GETFILE_OP)");
							   	errno=0;
							    munmap(mappedfile, MSG.data.hdr.len);
								setHeader(&reply_c.hdr,OP_FAIL,MSG.hdr.sender);
								res=sendHeader(fd_c,&reply_c.hdr);
								CONNCHECK(res,"sendHeader(3)(GETFILE_OP)");
							}
							if(replyLock != NULL) unlock(&(replyLock->fdLock));
							replyLock=NULL;
							munmap(mappedfile, conf.MaxFileSize);
						}
						MSG.data.buf = NULL;
						reply_c.data.buf = NULL;
					}
					FREE(fileName);
					fileName=NULL;
				}break;
				case USRLIST_OP:{
					if(icl_hash_find(hash,MSG.hdr.sender)!=NULL){//il sender è registrato
						reply_c.hdr.op = OP_OK;
						lock(&(online_c->onlineMTX));
						reply_c.data.hdr.len = nOnline(online_c);
						reply_c.data.hdr.len *= (MAX_NAME_LENGTH+1);
						lock(&(online_c->onlineMTX));
						//preparo la lista degli utenti online
						if((online_dump(online_c,&reply_c.data))==-1){
							fprintf(stderr, "ERRORE: online_dump(USRLIST_OP)\n");
							reply_c.hdr.op = OP_FAIL;
						}

						//mando la risposta OP_OK o OP_FAIL
						replyLock=searchAndLock(online_c,MSG.hdr.sender);
						res=sendHeader(fd_c,&reply_c.hdr);
						CONNCHECK(res,"sendHeader(USRLIST_OP)");

						//mando la lista degli utenti connessi
						if(reply_c.hdr.op == OP_OK){
							res=sendData(fd_c,&reply_c.data);
							CONNCHECK(res,"sendData(USRLIST_OP)");
						}
						else addToStat(&chattyStats.nerrors,1,1);
						if(replyLock != NULL) unlock(&(replyLock->fdLock));
						replyLock=NULL;
						FREE(reply_c.data.buf);
						reply_c.data.buf=NULL;
					}
					else{//non è registrato
						reply_c.hdr.op = OP_NICK_UNKNOWN;
						res=sendHeader(fd_c,&reply_c.hdr);
						CONNCHECK(res,"sendHeader(1)(USRLIST_OP)");
						addToStat(&chattyStats.nerrors,1,1);
					}
				}break;
				case UNREGISTER_OP:{
					//controllo che l'utente sia registrato
					if(icl_hash_find(hash,MSG.hdr.sender)!=NULL){
						reply_c.hdr.op = OP_OK;
						//cancello e invio la risposta al client
						if(!icl_hash_delete(hash,MSG.hdr.sender,free_key, free_data)){
							fprintf(stderr, "ERRORE: icl_hash_insert(UNREGISTER_OP)\n");
							reply_c.hdr.op = OP_FAIL;
						}
						replyLock=searchAndLock(online_c,MSG.hdr.sender);
						res=sendHeader(fd_c,&reply_c.hdr);
						if(replyLock != NULL) unlock(&(replyLock->fdLock));
						replyLock=NULL;
						CONNCHECK(res,"sendHeader(UNREGISTER_OP)");

						if(reply_c.hdr.op != OP_FAIL) addToStat(&chattyStats.nusers,1,0);
						else addToStat(&chattyStats.nerrors,1,1);
					}
					else{//utente non registrato
						printf("%s non è registrato\n",MSG.hdr.sender);
						reply_c.hdr.op = OP_NICK_UNKNOWN;
						res=sendHeader(fd_c,&reply_c.hdr);
						CONNCHECK(res,"sendHeader(1)(UNREGISTER_OP)");

						addToStat(&chattyStats.nerrors,1,1);
					}
				}break;
				case DISCONNECT_OP:{
					reply_c.hdr.op = OP_OK;
					if(online_remove(online_c,fd_c)==-1){
						fprintf(stderr, "ERRORE: online_remove(DISCONNECT_OP)\n");
						reply_c.hdr.op = OP_FAIL;
					}
					//non serve la lock
					res=sendHeader(fd_c,&reply_c.hdr);
					CONNCHECK(res,"sendHeader(DISCONNECT_OP)");

					addToStat(&chattyStats.nonline,1,0);
				}break;
				default:{
					fprintf(stderr, "ERRORE: messaggio non valido\n");
					return (void*)-1;
	    		}
	  		}
	  	}
	  	//la read non ha letto nulla
	  	else{
	  		//il client è terminato
	  		reply_c.hdr.op = OP_TERM;
	  		//rimuovo solo se lo avevo inserito
	  		online_remove(online_c,fd_c);
	  		FREE(MSG.data.buf);
	  		MSG.data.buf = NULL;
	  		//aggiorno le statistiche
	  		addToStat(&chattyStats.nonline,1,0);
	  	}
    	lock(&workedMTX);
    	if(newRequest(workedList,fd_c,reply_c.hdr.op)==-1){
			perror("ERRORE: newRequest");
			close(fd_c);
			unlock(&workedMTX);
			return (void*)0;
		}
		unlock(&workedMTX);
	}

	return (void*)0;
}
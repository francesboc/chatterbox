/*
* Autore: Francesco Bocchi
* Matricola: 550145
* Indirizzo e-mail: francesboc26@gmail.com
*
* Il progetto è stato realizzato in ogni sua parte da Francesco Bocchi.
*/

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<online_list.h>
#include<config.h>
#include<connections.h>

#define FREE(x) \
	if(x != NULL) free(x);

online_queue* initializeQueue(){
	online_queue* q = (online_queue*) malloc(sizeof(online_queue));
	if(!q) return NULL;
	q->head = NULL;
	q->online = 0;
	int err;
	if((err=pthread_mutex_init(&(q->onlineMTX), NULL)<0)){
        perror("impossibile inizializzare mutex onlineMTX");
        exit(errno);
    }
    return q;
}

int online_insert(online_queue* list,char* buf,int fd){
	int tmp=-1,err=0; 
	if(!buf) return tmp;
	lock(&(list->onlineMTX));
	online_users* new_user = calloc(1,sizeof(online_users));
	strncpy(new_user->buf,buf,strlen(buf));
	new_user-> fd_c = fd;
	new_user->next = list->head;
	if((err=pthread_mutex_init(&(new_user->fdLock), NULL)<0)){
        perror("impossibile inizializzare mutex user");
        exit(errno);
    }
	list->head=new_user;
	list->online++;
	tmp = list->online;
	//unlock(&(list->onlineMTX));
	return tmp;
}

int online_remove(online_queue* list,int fd){
	lock(&(list->onlineMTX));
	if(!list){
		unlock(&(list->onlineMTX));
		return -1;
	}
	online_users* prec = NULL;
	online_users* corr = list->head;
	while(corr != NULL){
		if(corr->fd_c==fd){
			if(prec == NULL){
				list->head = list->head->next;
				list->online--;
				pthread_mutex_lock(&(corr->fdLock));
				FREE(corr);
				unlock(&(list->onlineMTX));
				return 1;
			}
			else{
				prec->next = corr->next;
				list->online--;
				pthread_mutex_lock(&(corr->fdLock));
				FREE(corr);
				unlock(&(list->onlineMTX));
				return 1;
			}
		}
		prec = corr;
		corr=corr->next;
	}
	unlock(&(list->onlineMTX));
	return -1;
}

int online_find(online_queue* list,char* buf){
	if(!buf) return -1;

	lock(&(list->onlineMTX));
	online_users* corr= list->head;
	while(corr != NULL){
		if(strcmp(corr->buf,buf)==0){
			unlock(&(list->onlineMTX));
			return corr->fd_c;
		}
		corr=corr->next;
	}
	unlock(&(list->onlineMTX));
	return -1;
}

online_users* searchAndLock(online_queue* list,char* buf){
	lock(&(list->onlineMTX));
	online_users* corr= list->head;
	while(corr != NULL){
		if(strcmp(corr->buf,buf)==0){
			lock(&(corr->fdLock));
			unlock(&(list->onlineMTX));
			return corr;
		}
		corr=corr->next;
	}
	unlock(&(list->onlineMTX));
	return NULL;
}

int getOnlineList(online_queue*list,online_users* tmp){
	int i=0;
	lock(&(list->onlineMTX));
	if(!list){
		unlock(&(list->onlineMTX));
		return i;
	}
	online_users* corr= list->head;
	while(corr != NULL){
		tmp[i].fd_c=corr->fd_c;
		strcpy(tmp[i].buf,corr->buf);
		i++;
		corr=corr->next;
	}
	unlock(&(list->onlineMTX));
	return i;
}

int nOnline(online_queue* list){
	int tmp=0;
	//lock(&(list->onlineMTX));
	tmp = list->online;
	unlock(&(list->onlineMTX));
	return tmp;
}

int online_destroy(online_queue* list){
	if(!list) return -1;

	online_users* corr= list->head;
	while(corr != NULL){
		list->head=list->head->next;
		FREE(corr);
		corr = list->head;
	}
	return 1;
}

void online_print(online_queue* list){
	online_users* corr = list->head;
	printf("(nome,file_descriptor)\n");
	while(corr != NULL){
		printf("(%s,%d) -> ",corr->buf,corr->fd_c);
		corr=corr->next;
	}
	printf("\n");
}

int online_dump(online_queue* list,message_data_t* msg){
	//lock(&(list->onlineMTX));
	if(!list){
		unlock(&(list->onlineMTX));
		return -1;
	}
	msg->buf = (char*) calloc(msg->hdr.len,sizeof(char));
	
	online_users* corr= list->head;
	int i=0,j=1,k;
	while(corr != NULL){
		int n = j*(MAX_NAME_LENGTH+1);
		k=0;
		while(i<n){
			if(k < strlen(corr->buf))//copio l'utente
				msg->buf[i]=corr->buf[k];
			else msg->buf[i]='\0';//altrimenti riempio con fine stringa
			i++;
			k++;
		}
		j++;
		corr=corr->next;
	}
	unlock(&(list->onlineMTX));
	return 1;
}
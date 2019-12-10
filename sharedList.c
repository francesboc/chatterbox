/*
* Autore: Francesco Bocchi
* Matricola: 550145
* Indirizzo e-mail: francesboc26@gmail.com
*
* Il progetto è stato realizzato in ogni sua parte da Francesco Bocchi.
*/

#include <sharedList.h>
#include <stdio.h>
#include <stdlib.h>

#define FREE(x) \
	if(x != NULL) free(x);

queue* initQueue(){
	queue* q = malloc(sizeof(queue));
	if(!q) return NULL;
	q->head=NULL;
	q->tail=NULL;
	q->len=0;
	return q;
}

int newHistoryInsert(queue* q, message_t msg,int histSize,int check){
	request* new_req = malloc(sizeof(request));
	if(!new_req) return -1;
	new_req -> fd_c = -1;
	new_req -> op 	= -1;
	new_req -> msg.hdr.op = msg.hdr.op;
	new_req -> check = check;
	strcpy(new_req->msg.hdr.sender,msg.hdr.sender);
	new_req->msg.data.buf = malloc(msg.data.hdr.len*sizeof(char));
	strncpy(new_req->msg.data.buf,msg.data.buf,msg.data.hdr.len);
	strcpy(new_req->msg.data.hdr.receiver,msg.data.hdr.receiver);
	new_req->msg.data.hdr.len=msg.data.hdr.len;
	new_req -> next	= NULL;

	if(q->tail == NULL){
		q->head = new_req;
		q->tail = new_req;
	}
	else{
		q->tail->next= new_req;
		q->tail = new_req;
	}
	if(q->len == histSize){
		request* corr = q->head;
		q->head = q->head->next;
		FREE(corr->msg.data.buf);
		FREE(corr);
	}
	else q->len++;
	return 1;
}

void stampaHistory(queue* q){
	request* corr = q->head;
	while(corr != NULL){
		printf("---------------->%s\n",corr->msg.data.buf);
		corr=corr->next;
	}
}

void freeHistory(queue* q){
	request* corr = q->head;
	while(corr != NULL){
		q->head=q->head->next;
		q->len--;
		FREE(corr->msg.data.buf);
		corr->msg.data.buf=NULL;
		FREE(corr);
		corr=q->head;
	}
}

int getElem(queue* q,message_t* msg,int* count,int* fileCount){
	request* corr = q->head;
	int i=0;
	while(corr!=NULL){
		msg[i].hdr.op = corr->msg.hdr.op;
		strcpy(msg[i].hdr.sender,corr->msg.hdr.sender);
		msg[i].data.buf = calloc(corr->msg.data.hdr.len,sizeof(char));
		strncpy(msg[i].data.buf,corr->msg.data.buf,corr->msg.data.hdr.len);
		strcpy(msg[i].data.hdr.receiver,corr->msg.data.hdr.receiver);
		msg[i].data.hdr.len=corr->msg.data.hdr.len;
		if(corr->check == 1){
			if(corr->msg.hdr.op==FILE_MESSAGE)
				(*fileCount)++;
			else (*count)++;
			corr->check=0;
		}
		corr=corr->next;
		i++;
	}
	return i;
}

int newRequest(queue* q, int fd, op_t op){
	if(fd<0) return -1;

	request* new_req = (request*) malloc(sizeof(request));
	new_req -> fd_c = fd;
	new_req -> next = NULL;
	//se op == -1 significa che la richiesta proviene dal listener
	if(op != -1)//richiesta inviata da worker a listener (equivale all'esito dell' operazione)
		new_req -> op = op;
	if(q->tail == NULL){//lista richieste vuota
		q->head = new_req;
		q->tail = new_req;
	}
	else{//c'è almeno un elemento
		q->tail->next = new_req;
		q->tail = new_req;
	}
	q->len++;
	return 1;
}

int getRequest(queue* q,request* arg){
	if(q->head == NULL) return -1;
	request* corr = q->head;
	(*arg).fd_c = q -> head -> fd_c;
	(*arg).op = q -> head -> op;
	q->head = q-> head -> next;
	if(q -> tail -> fd_c == corr->fd_c)//c'era solo un elemento nella lista
		q->tail = NULL;

	q->len--;
	FREE(corr);
	return 1;
}

int destroyRequest(queue* q){
	if(q == NULL || q->head == NULL) return -1;

	request* corr = q->head;

	while(corr != NULL){
		q->head = q->head -> next;
		FREE(corr);
		corr = q->head;
	}
	return 1;
}

void printRequest(queue* q){
	request* corr = q->head;
	printf("La lista delle richieste è\n");
	while(corr != NULL){
		printf("%d\n",corr->fd_c);
		corr = corr->next;
	}
}
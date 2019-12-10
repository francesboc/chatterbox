/*
* Autore: Francesco Bocchi
* Matricola: 550145
* Indirizzo e-mail: francesboc26@gmail.com
*
* Il progetto è stato realizzato in ogni sua parte da Francesco Bocchi.
*/

#include<connections.h>
#include <help.h>

int openConnection(char* path, unsigned int ntimes, unsigned int secs){
	//ho passato un numero di tentativi maggiori di MAX_RETRIES
	if(ntimes > MAX_RETRIES){
		perror("Numero di tentativi troppo alto");
		return -1;
	}
	//ho passato un numero di secondi maggiore di MAX_SLEEPING
	if(secs > MAX_SLEEPING){
		perror("Tempo di attesa troppo alto");
		return -1;
	}
	//descrittore socket
	int fd_skt;
	struct sockaddr_un sa;
	strncpy(sa.sun_path,path,UNIX_PATH_MAX);
	sa.sun_family = AF_UNIX;
	SYSCALL(fd_skt,socket(AF_UNIX,SOCK_STREAM,0),"errore socket");
	
	int i=0;
	
	/* provo a connettermi finchè la connect non va a buon fine,
	 * oppure finchè ho effettuato un numero di tentativi minore
	 * di ntimes */

	while(connect(fd_skt,(struct sockaddr*)&sa,sizeof(sa))==-1 && i<ntimes){
		if(errno == ENOENT){
			i++;
			sleep(secs);
		}
		else return -1;
	}
	if(i<ntimes) return fd_skt;
	else return -1;
}

int readHeader(long connfd, message_hdr_t *hdr){
	int err=0;
	if((err=readn(connfd,hdr,sizeof(message_hdr_t)))>=0)
		return err;
	perror("readHeader");
	return -1;
}

int readData(long fd, message_data_t *data){
	int err=0;
	if((err=readn(fd,&data->hdr,sizeof(message_data_hdr_t)))<0){
		perror("readData(1)");
		return -1;
	}
	if(data->hdr.len > 0){
		if(data->buf != NULL)
			data->buf = NULL;
		data->buf = calloc(data->hdr.len,sizeof(char));
		if(!data->buf){
			perror("readData(2)");
			return -1;
		}
		if((err=readn(fd,data->buf,data->hdr.len))<0){
			perror("readData(3)");
			return -1;
		}
	}
	return err;
}

int readMsg(long fd, message_t *msg){
	if(readHeader(fd,&msg->hdr)<0) return -1;
	int r=readData(fd,&msg->data);
	if(r<=0){
		if(r == 0) return 0;
		else return -1;
	}
	return 1;
}

int sendData(long fd, message_data_t *msg){
	int err=0;
	if((err=writen(fd,&msg->hdr,sizeof(message_data_hdr_t)))<0){
		perror("sendData(1)");
		return -1;
	}
	if((err=writen(fd,msg->buf,msg->hdr.len))<0){
			perror("sendData(2)");
			return -1;
	}
	return err;
}

int sendHeader(long fd,message_hdr_t* msg){
	int err=0;
	if((err=writen(fd,msg,sizeof(message_hdr_t)))>=0)
		return err;
	perror("sendHeader");
	return -1;
}

int sendRequest(long fd, message_t *msg){
	if(sendHeader(fd,&msg->hdr)<0) return -1;
	if(sendData(fd,&msg->data)<0) return -1;
	return 1;
}
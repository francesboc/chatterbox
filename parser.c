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
#include<parser.h>

void check_row(configuration* conf,char* buf){
	char* token = NULL;
	char* tmpstr = NULL;
	
	token=__strtok_r(buf,"=",&tmpstr);

	if(strncmp(token,"UnixPath",strlen("UnixPath"))==0)
		strncat(conf->UnixPath,tmpstr+1,strlen(tmpstr));
	else if(strncmp(token,"MaxConnections",strlen("MaxConnections"))==0)
		conf->MaxConnections=atoi(tmpstr+1);
	else if(strncmp(token,"ThreadsInPool",strlen("ThreadsInPool"))==0)
		conf->ThreadsInPool=atoi(tmpstr+1);
	else if(strncmp(token,"MaxMsgSize",strlen("MaxMsgSize"))==0)
		conf->MaxMsgSize=atoi(tmpstr+1);
	else if(strncmp(token,"MaxFileSize",strlen("MaxFileSize"))==0)
		conf->MaxFileSize=atoi(tmpstr+1);
	else if(strncmp(token,"MaxHistMsgs",strlen("MaxHistMsgs"))==0)
		conf->MaxHistMsgs=atoi(tmpstr+1);
	else if(strncmp(token,"DirName",strlen("DirName"))==0)
		strncat(conf->DirName,tmpstr+1,strlen(tmpstr)-2);
	else if(strncmp(token,"StatFileName",strlen("StatFileName"))==0)
		strncat(conf->StatFileName,tmpstr+1,strlen(tmpstr));

}

void parser(configuration* conf,char* argv){
	char tmp[MAXDIM] = "./";
	FILE *ifp=NULL;

	strcpy((*conf).UnixPath,"");
	(*conf).MaxConnections = 0;
	(*conf).ThreadsInPool = 0;
	(*conf).MaxMsgSize = 0;
	(*conf).MaxFileSize = 0;
	(*conf).MaxHistMsgs = 0;
	strcpy((*conf).DirName,"");
	strcpy((*conf).StatFileName,"");

	//APRO IL FILE DI CONFIGURAZIONE DA PARSARE
	if((ifp = fopen(strncat(tmp,argv,MAXDIM),"r"))==NULL){
		perror("Errore apertura file");
		return ;
	}
	char* buf = calloc(MAXLINE,sizeof(char));
	//controllo ogni riga con la funzione check_row
	while((fgets(buf,MAXLINE,ifp))!= NULL){
		if(buf[0] != '#' && buf[0] != ' ' && buf[0] != '\n'){
			buf[strlen(buf)-1]='\0';
			check_row(conf,buf);
		}
	}
	if(buf) free(buf);
	fclose(ifp);
}
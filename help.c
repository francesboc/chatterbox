/*
* Autore: Francesco Bocchi
* Matricola: 550145
* Indirizzo e-mail: francesboc26@gmail.com
*
* Il progetto è stato realizzato in ogni sua parte da Francesco Bocchi.
*/

#include<help.h>

void free_key(void* key){
    free(key);
}

void free_data(void* data){
    free(data);
}

int readn(long fd, void *buf, size_t size){
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
    if ((r=read((int)fd ,bufptr,left)) == -1) {
        if (errno == EINTR) continue;
        if (errno == ECONNRESET) return 0;
        return -1;
    }
    if (r == 0) return 0;   // gestione chiusura socket
        left    -= r;
    bufptr  += r;
    }
    return size;
}

int writen(long fd, void *buf, size_t size){
    size_t left = size;
    int r;
    char *bufptr = (char*)buf;
    while(left>0) {
    if ((r=write((int)fd ,bufptr,left)) == -1) {
        if (errno == EINTR) continue;
        if (errno == EPIPE) return 0;
        return -1;
    }
    if (r == 0) return 0;  
        left    -= r;
    bufptr  += r;
    }
    return 1;
}
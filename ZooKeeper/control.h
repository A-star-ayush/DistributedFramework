#ifndef _CONTROL_H
#define _CONTROL_H

#include <time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

struct address_w {
	time_t timestamp;
	struct in_addr ip;
	short cport, dport;
};

struct fList{
	char* name;
	int indexes[10];  // maximum 10 locations per file will be stored
};						// it is the duty of the distributed hashing algorithm to distribute the file evenly

extern int nWorkers;				extern pthread_mutex_t sz;
extern struct address_w* Adds;		extern pthread_mutex_t ad;
extern struct fList* fileList;		extern pthread_mutex_t fl;
extern int fsize;

void* maintainControlPort(void* pNum);

#define lock pthread_mutex_lock
#define trylock pthread_mutex_trylock
#define unlock pthread_mutex_unlock

#endif
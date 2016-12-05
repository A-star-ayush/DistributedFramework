#include <stdint.h>
#include <unistd.h>
#include <strings.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include "control.h"
#include "utility.h"
#include "log.h"

static int asize = 0;
int fsize = 0;

int nWorkers = 1; 					pthread_mutex_t sz = PTHREAD_MUTEX_INITIALIZER; // the corresponding mutexes
struct address_w* Adds = NULL;      pthread_mutex_t ad = PTHREAD_MUTEX_INITIALIZER;
struct fList* fileList = NULL;		pthread_mutex_t fl = PTHREAD_MUTEX_INITIALIZER;

struct fd_wnum {
	int fd, workerNum;
};

void resizeAddressList(int sz){  // if time permits devise a mechanism to release back heap memory when workers leave
	--sz;   // rounding sz to the next power of 2
		sz |= sz >> 1;
		sz |= sz >> 2;
		sz |= sz >> 4;
		sz |= sz >> 8;
		sz |= sz >> 16;
	++sz;
	
	lock(&ad);
	Adds = realloc(Adds, sizeof(struct address_w)*sz);
	if(Adds == NULL) exit_err("resize:realloc:Address");
	unlock(&ad);

	asize = sz;
	char msg[32];
	sprintf(msg, " addressList resized to %d ", sz);
	stamp(msg);
}

static void resizeFileList(int sz){
	--sz;
		sz |= sz >> 1;
		sz |= sz >> 2;
		sz |= sz >> 4;
		sz |= sz >> 8;
		sz |= sz >> 16;
	++sz;
	
	struct fList* fileList_cpy = calloc(sz, sizeof(struct fList));
	if(fileList_cpy == NULL) exit_err("resize:calloc:fileList_cpy");
	lock(&fl);
	if(fileList != NULL){
	 	memcpy(fileList_cpy, fileList, sizeof(struct fList)*fsize);
	 	free(fileList);
	}
	fileList = fileList_cpy;
	fsize = sz;
	unlock(&fl);
	
	char msg[30];
	sprintf(msg, " fileList resized to %d ", sz);
	stamp(msg);
}

static void* heartbeat(void* _fd){
	char buf[10];
	int fd = (intptr_t)_fd;

	if(pthread_detach(pthread_self())!=0) exit_on_error("getfilesinfo:pthread_detach");

	struct sockaddr_in padd;
	struct timeval timeout;      
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    int rt = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, (char *)&timeout, sizeof(timeout));
    if(rt < 0) exit_on_error("heartbeat: setsockopt");

	int sz = sizeof(struct sockaddr_in);
	int i;
	while(1){
		rt = recvfrom(fd, buf, 10, 0, aCast(&padd), &sz);
		if(rt < 0 && errno!=EWOULDBLOCK) exit_on_error("heartbeat: recvfrom");

		if(rt > 0) stamp3(" recevied heartbeat ", &padd);

		time_t now = time(NULL);

		for(i=1;i<nWorkers;++i){
			if(Adds[i].cport > 0){  // we set cport = 0 for the non-active workers
				if(rt > 0 && padd.sin_addr.s_addr == Adds[i].ip.s_addr && padd.sin_port == Adds[i].cport){
					Adds[i].timestamp = now;
				}
				else{
					if(now - Adds[i].timestamp > 3){
						lock(&ad);
						struct sockaddr_in wadd;
						wadd.sin_family = AF_INET;
						wadd.sin_port = Adds[i].cport;
						wadd.sin_addr = Adds[i].ip;

						Adds[i].cport = 0;
						
						stamp3(" worker dead ", &wadd);
						unlock(&ad);
					}
				}
			}
		}
	}
	return NULL;
}

static void* getInfo(void* arg){
	char* buf = malloc(BUFSIZ);
	char* buf_cpy = buf;  // the buf pointer gets moved around and that will cause problem with the free command

	if(pthread_detach(pthread_self())!=0) exit_on_error("getfilesinfo:pthread_detach");

	int fd = ((struct fd_wnum*)arg)->fd;
	int wnum = ((struct fd_wnum*)arg)->workerNum;
	free(arg);

	int rt = write(fd, "GIVE DPORT", 13);
	if(rt < 0) exit_on_error("getInfo:write");

	struct sockaddr_in Add;
	{
		int size = sizeof(struct sockaddr_in);
		bzero(&Add, size);
		if(getpeername(fd, aCast(&Add), &size)==-1) { close(fd); return NULL; }
		Adds[wnum].timestamp = time(NULL);
		Adds[wnum].ip = Add.sin_addr;
		Adds[wnum].cport = Add.sin_port;

		rt = read(fd, buf, BUFSIZ);
		if(rt < 0) exit_on_error("getInfo:read");
		long dport = strtol(buf, &buf, 10);
		if((*buf)!='\0') exit_err("getInfo:getfiles:endpoint mismatch_msg");
		Adds[wnum].dport = htons(dport);  // storing the port values in network endianess
		
		rt = write(fd, "OK", 3);
		if(rt < 0) exit_on_error("getInfo:write");

		stamp2(" data port info recevied from worker ", fd, 1);
	}

	rt = write(fd, "GIVE FNAMES", 12);
	if(rt < 0) exit_on_error("getInfo:write");

	long turns = 1;
	while(1){
		rt = read(fd, buf, BUFSIZ);
		if(rt < 0) exit_on_error("getInfo:read");
		if(rt==0) break;
		turns = strtol(buf, &buf, 10);
		if(errno == ERANGE) exit_on_error("getInfo:strtol:turns");
		long nfiles = strtol(buf+1,&buf, 10);
		if(errno == ERANGE) exit_on_error("getInfo:strtol:nfiles");
		if(nfiles > fsize) resizeFileList(nfiles);
		int i, j;
		long fnsize = 0;
		lock(&fl);
		for(i=0;i<nfiles;++i){
			fnsize = strtol(buf+fnsize+1,&buf, 10);
			char file[fnsize+1];
			strncpy(file, buf+1, fnsize);
			file[fnsize] = '\0';
			j = 0;
			while(j < fsize){
				if(fileList[j].name == NULL){
					fileList[j].name = malloc(fnsize+1);
					strcpy(fileList[j].name, file);
					fileList[j].indexes[0] = wnum;
					break;
				}
				if(strcasecmp(fileList[j].name, file)==0){
					int k;
					for(k=0;k<10;++k) 
						if(fileList[j].indexes[k]==0){
						 	fileList[j].indexes[k] = wnum;
						 	break;
						}
					break;
				}
				++j;
			}
			if(j == fsize){
				int sz_cpy = fsize;
				unlock(&fl);  // otherwise you will have a deadlock
				resizeFileList(fsize*2);
				lock(&fl);
				fileList[sz_cpy].name = malloc(fnsize+1);
				strcpy(fileList[sz_cpy].name, file);
				fileList[sz_cpy].indexes[0] = wnum;
			}
		}
		unlock(&fl);

		if(!turns) break;
	}
	free(buf_cpy);

	if(rt > 0){
		rt = write(fd, "OK", 3);
		if(rt < 0) exit_on_error("getInfo:write");
	}

	if(rt > 0) stamp2(" file names info recevied from worker ", fd, 1);
	else {
		stamp3(" worker dead ", &Add);
		Adds[wnum].cport = -1;  // to indicate that the worker is dead
	}
	
	close(fd);

	return NULL;
}

void* maintainControlPort(void* cport){
	
	if(pthread_detach(pthread_self())!=0) exit_on_error("maintainControlPort:pthread_detach");
	
	int soc;

	{	
		int pNum = (intptr_t)cport;
		soc = createSocket(1, AF_INET, pNum, INADDR_ANY, 5);
		stamp2(" tcp socket created ", soc, 0);
	}
	
	{
		int pNum = (intptr_t)cport;
		int soc2 = createSocket(0, AF_INET, pNum, INADDR_ANY, 0);
		stamp2(" udp socket created ", soc2, 0);
		pthread_t t;
		if(pthread_create(&t, NULL, heartbeat, (void*)(intptr_t)soc2)<0)
			exit_err("maintainControlPort:pthread_create:heartbeat thread");
		stamp(" heartbeat thread created ");
	}
	
	while(1){
		int worker = accept(soc, NULL, NULL);
		if(worker < 0) exit_on_error("maintainControlPort:accept");
		stamp2(" worker connected ", worker, 1);

		lock(&sz);
		++nWorkers;
		if(nWorkers > asize) resizeAddressList(nWorkers);
		unlock(&sz);

		{
			pthread_t t;
			struct fd_wnum* data = malloc(sizeof(struct fd_wnum));
			data->fd = worker;
			data->workerNum = nWorkers-1;
			if(pthread_create(&t, NULL, getInfo, data)!=0)
				exit_err("maintainControlPort:pthread_create:getInfo thread");
			stamp2(" info thread created for worker ", worker, 1);
		}
	}


	return NULL;
}

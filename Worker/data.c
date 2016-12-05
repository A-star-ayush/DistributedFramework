#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include "data.h"
#include "log.h"
#include "utility.h"
#include "control.h"
#include "solution.h"

short DPORT = 20000;

#define encountered_error(xmsg) \
	do { fprintf(stderr, "error at %s\n", xmsg); char msg[rt+58]; sprintf(msg," encountered error while computing GCP on %s ", fname); stamp(msg); } while(0)

void* doGCP(void* _fd){
	if(pthread_detach(pthread_self())!=0) exit_on_error("doTask:pthread_detach");
	int fd = (intptr_t)_fd;
	char buf[BUFSIZ];
	
	{
		int val = 0;
		ioctl(fd, FIONBIO, &val);
	}
	
	int rt = write(fd, "OK", 3);
	if(rt < 0) exit_on_error("doGCP:read");
	if(rt == 0) { close(fd); return NULL; }

	rt = read(fd, buf, BUFSIZ);
	if(rt < 0) exit_on_error("doGCP:read");
	if(rt == 0) { close(fd); return NULL; }

	char fname[rt+8];
	strcpy(fname,"shared/");
	strncat(fname, buf, rt);
	
	
	{
		char msg[rt+38];
		sprintf(msg," started computing GCP on %s ", fname);
		stamp(msg);
	}

	int in = open(fname, O_RDONLY);
	if(in <= 0){  encountered_error("doGCP:open"); close(fd); return NULL; }

	int k = findColoring(in, fd);
	if(k==0) { encountered_error("findColoring"); close(fd); return NULL; }
	
	{
		char msg[rt+38];
		sprintf(msg," done computing GCP on %s ", fname);
		stamp(msg);
	}

	close(fd);
	return NULL;
}
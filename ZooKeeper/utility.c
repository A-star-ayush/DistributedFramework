#include "utility.h"
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <strings.h>
#include <string.h>
#include <sys/ioctl.h>

int createSocket(int type, int aFm, short pNum, int bAdd, int nListen){
	struct sockaddr_in lAdd;
	int fd, rt, sz = sizeof(struct sockaddr_in);
	{
		int M, socType;
		M = -(type==1);
		socType = (SOCK_STREAM & M) | (SOCK_DGRAM & ~M);
		fd = socket(aFm, socType, 0);
	}

	{
		long state = 1;  // this has to be done before binding
 		if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &state, sizeof(int)) < 0) exit_on_error("setsockopt");
 	}
 	
	if(fd==-1) exit_on_error("createSocket:socket");
	bzero(&lAdd, sz);
	lAdd.sin_family = aFm;
	lAdd.sin_port = htons(pNum);
	lAdd.sin_addr.s_addr = bAdd;
	rt = bind(fd, aCast(&lAdd), sz);
	if(rt==-1) exit_on_error("createSocket:bind");
	if(type==1 && nListen > 0) { rt = listen(fd, nListen);  // listen operation not supported for UDP
				  if(rt==-1) exit_on_error("createSocket:listen"); }
	
	int val = 0;
    ioctl(fd, FIONBIO, &val);  // enabling BLOCKING mode I/O on cfd

	return fd;
}

char* addressString(int soc, int type){
	static char add[25];
	int sz = sizeof(struct sockaddr_in);
	struct sockaddr_in Add;
	bzero(&Add, sz);

	if(type == 0 && getsockname(soc, aCast(&Add), &sz)==-1) exit_on_error("addressString:getsockname");
	if(type > 0 && getpeername(soc, aCast(&Add), &sz)==-1) exit_on_error("addressString:getpeername");
	
	char* ip = inet_ntoa(Add.sin_addr);
	strcpy(add, ip);
	strcat(add,",");
	char port[5];
	if(snprintf(port, 6, "%d", ntohs(Add.sin_port)) < 0) exit_on_error("addressString:snprintf");
	strcat(add, port);
	return add;
}

char* addressString2(void* _add){
	static char add[25];
	int sz = sizeof(struct sockaddr_in);
	struct sockaddr_in* Add = (struct sockaddr_in*)_add;
	char* ip = inet_ntoa(Add->sin_addr);
	strcpy(add, ip);
	strcat(add,",");
	char port[5];
	if(snprintf(port, 6, "%d", ntohs(Add->sin_port)) < 0) exit_on_error("addressString2:snprintf");
	strcat(add, port);
	return add;	 
}
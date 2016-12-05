#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
#include <unistd.h>
#include <dirent.h>
#include "control.h"
#include "utility.h"
#include "log.h"
#include "data.h"

struct args {
	struct sockaddr_in Add;
	int fd;
};

short CPORT = 40000;

static void writeFileNames(int fd){
	struct dirent** namelist; 
	int ele = scandir("shared", &namelist, NULL, alphasort);  
	if(ele < 0) exit_on_error("writeFileNames:scandir");
	int i;
	char buf[BUFSIZ];  // here we are assuming that the file names will be accomodated in 1 turn later on we can extend it
	int n = sprintf(buf,"%d %d ", 0, ele-2);  // first two entries are "." and ".."
	for(i=2;i<ele;i++){
		int len = strlen(namelist[i]->d_name);
		n = sprintf(buf+n, "%d ", len) + n;
		n = sprintf(buf+n, "%s ", namelist[i]->d_name) + n;
	}

	int rt = write(fd, buf, BUFSIZ);
	if(rt < 0) exit_on_error("writeFileNames:write");
}

static void* heartbeat(void* arg){
	if(pthread_detach(pthread_self())!=0) exit_on_error("getfilesinfo:pthread_detach");
	
	int fd = ((struct args*)arg)->fd;
	struct sockaddr_in to = ((struct args*)arg)->Add;
	int sz = sizeof(struct sockaddr_in);

	while(1){
		sleep(2);
		int rt = sendto(fd, "", 1, 0, aCast(&to), sz);
		if(rt < 0) exit_on_error("heartbeat:sendto");
		stamp3(" sent heartbeat ", &to);
	}

	return NULL;
}

void* contact(void* arg){

	int soc = createSocket(1, AF_INET, CPORT, INADDR_ANY, -1);
	stamp2(" tcp socket created ", soc, 0);
	
	struct args para;

	{
		struct sockaddr_in Add;
		int sz = sizeof(struct sockaddr_in);
		bzero(&Add, sz);
		Add.sin_family = AF_INET;
		Add.sin_port = htons(((struct arg_data*)arg)->port);
		Add.sin_addr = ((struct arg_data*)arg)->ip;

		int rt = connect(soc, aCast(&Add), sz);
		if(rt < 0) exit_on_error("contact:connect");

		stamp3(" connected to Zookeeper ", &Add);
		
		para.Add = Add;
	}

	{
		int soc2 = createSocket(0, AF_INET, CPORT, INADDR_ANY, 0);
		stamp2(" udp socket created ", soc2, 0);

		para.fd = soc2;
		pthread_t t;
		if(pthread_create(&t, NULL, heartbeat, &para)<0)
			exit_err("maintainControlPort:pthread_create:heartbeat thread");
		stamp(" heartbeat thread created ");
	}

	while(1){
		char buf[BUFSIZ];
		int rt = read(soc, buf, BUFSIZ);
		if(rt < 0) exit_on_error("contact:read");
		if(rt == 0) break;
		if(strncasecmp(buf, "GIVE", 4)==0){
			if(strncasecmp(buf+5, "DPORT", 5)==0){
				int n = sprintf(buf, "%d", DPORT);
				rt = write(soc, buf, n+1);
				if(rt < 0) exit_on_error("contact:write:dport");
				rt = read(soc, buf, BUFSIZ);
				if(rt < 0) exit_on_error("contact:read:dportOK");
				if(strcasecmp(buf, "OK")==0) stamp(" provided data port info ");
				else exit_err("contact:read:acknowledgement not OK");		
			}
			else if(strncasecmp(buf+5,"FNAMES",6)==0){
				writeFileNames(soc);
				rt = read(soc, buf, BUFSIZ);
				if(rt < 0) exit_on_error("contact:read:dportOK");
				if(strcasecmp(buf, "OK")==0) stamp(" provided file names info ");
				else exit_err("contact:read:acknowledgement not OK");		
			}
			else exit_err("contact:GIVE parameter not recognized"); 
		}
		else exit_err("contact:command not recognized");
	}

	close(soc);

	return NULL;
}

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <strings.h>
#include <pthread.h>
#include <stdint.h>
#include <errno.h>
#include "log.h"
#include "utility.h"
#include "control.h"
#include "data.h"

int main(int argc, char *argv[])
{
	if(argc < 5) exit_err("Insufficient arguments");

	initStamp("log.txt");
	stamp(" started session ");

	struct arg_data arg;
	{
		if(inet_aton(argv[1], &arg.ip)==0) exit_err("main:inet_aton");
		arg.port = strtol(argv[2], NULL, 10);
		if(arg.port == 0 || errno == ERANGE) exit_err("main:strtol:zookeeper's cport");
		
		CPORT = strtol(argv[3], NULL, 10);
		if(CPORT == 0 || errno == ERANGE) exit_err("main:strtol:cport");
		DPORT = strtol(argv[4], NULL, 10);
		if(DPORT == 0 || errno == ERANGE) exit_err("main:strtol:dport");
		
		pthread_t control;
		if(pthread_create(&control, NULL, contact, &arg)!=0)
			exit_err("main:pthread_create:control:non-zero return value");
		stamp(" created control thread ");
	}

	int soc = createSocket(1, AF_INET, DPORT, INADDR_ANY, 5);
	while(1){
		char buf[BUFSIZ];

		struct sockaddr_in sadd;
		int sz = sizeof(struct sockaddr_in);
		bzero(&sadd, sz);
		int cli = accept(soc, aCast(&sadd), &sz);
		if(cli < 0) exit_on_error("main:accept");
		stamp3(" accepted connection from ", &sadd);

		int rt = read(cli, buf, BUFSIZ);
		if(rt < 0) exit_on_error("main:read");
		if(rt == 0) { close(cli); stamp3(" lost connection to ", &sadd); continue; }

		if(strcasecmp(buf,"GCP")==0){
			pthread_t t;
			if(pthread_create(&t, NULL, doGCP, (void*)(intptr_t)cli)!=0)
				exit_err("main:pthread_create:doGCP:non-zero return value");
		}
		else exit_err("unknown command");
	}

	return 0;
}

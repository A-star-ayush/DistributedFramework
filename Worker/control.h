#ifndef _CONTROL_H
#define _CONTROL_H

#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>

struct arg_data{
	struct in_addr ip;
	short port;
};

extern short CPORT;

void* contact(void* _port);

#endif
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include "log.h"
#include "utility.h"

static int logfd;  // the log file's file descriptor

void initStamp(char* fname){
	if(fname==NULL) exit_err("initStamp: log file name cannot be NULL.");
	logfd = open(fname, O_RDWR | O_CREAT | O_TRUNC, 0666);
	if(logfd < 0) exit_on_error("initStamp:open");
}

void stamp(char* msg){
	if(msg==NULL) exit_err("stamp: message cannot be NULL.");
	static char* delim = " : ";
	static char* newLine = "\r\n";

	time_t t = time(NULL);
	char* ct = ctime(&t);
	int rt = write(logfd, ct, strlen(ct)-1);
	if(rt<=0) exit_on_error("stamp:write");
	rt = write(logfd, delim, 3);
	if(rt<=0) exit_on_error("stamp:write");
	rt = write(logfd, msg, strlen(msg));
	if(rt<=0) exit_on_error("stamp:write");
	rt = write(logfd, newLine, 2);
	if(rt<=0) exit_on_error("stamp:write");
}

void stamp2(char* msg, int soc, int type){
	char str[strlen(msg) + 30];

	strcpy(str, msg);
	strcat(str, "<");
	strcat(str, addressString(soc, type));
	strcat(str, ">");

	stamp(str);
}

void stamp3(char* msg, void* add){
	char str[strlen(msg) + 30];

	strcpy(str, msg);
	strcat(str, "<");
	strcat(str, addressString2(add));
	strcat(str, ">");

	stamp(str);
}
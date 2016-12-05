#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <pthread.h>
#include <stdint.h>
#include <string.h>
#include "log.h"
#include "control.h"
#include "utility.h"
#include "data.h"

int main(int argc, char const *argv[])
{
	initStamp("log.txt");
	stamp(" started session ");

	{
		int CPORT = 40000;
		pthread_t control;
		if(pthread_create(&control, NULL, maintainControlPort, (void*)(intptr_t)CPORT)!=0)
			exit_err("main:pthread_create:control:non-zero return value");
		stamp(" created control thread ");
	}

	char input[100];

	while(1){
		static int i = 0;
		scanf("%100s", input);
		if(strcasecmp(input, "EXT")==0) break;
		else if(strcasecmp(input, "GCP")==0){
			struct taskD* x = malloc(sizeof(struct taskD));
			if(x == NULL) exit_err("main:malloc:returned NULL for task description");
			strcpy(x->taskCode,"GCP");
			int nfiles;
			int rt = scanf("%d",&nfiles);
			if(rt == 0) exit_err("main:scanf:nfiles:malformed command");
			x->files = malloc(sizeof(char*)*nfiles);
			if(x->files == NULL) exit_err("main:malloc:returned NULL for files");
			x->nfiles = nfiles;
			x->taskNum = ++i;
			int i;
			for(i=0;i<nfiles;++i){
				scanf("%100s",input);  // maximum length of filename restricted to 100
				x->files[i] = malloc(strlen(input)+1);
				if(x->files[i] == NULL) exit_err("main:malloc:returned NULL for files[i]");
				strcpy(x->files[i], input);
			}
			char msg[40];
			sprintf(msg, " GCP command received: TASK %d ", x->taskNum);
			stamp(msg);		
			pthread_t data;
			if(pthread_create(&data, NULL, doTask, x)!=0)
				exit_err("main:pthread_create:data:non-zero return value");
			stamp(" created data thread ");
		}
		else exit_err("main:unrecognized command");  // our instruction set currently contains only EXT and GCP
	}

	return 0;
}

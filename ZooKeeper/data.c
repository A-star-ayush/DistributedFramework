#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include "control.h"
#include "utility.h"
#include "log.h"
#include "data.h"

struct task{
	char* fname;
	char* outFile;
	int wnum;
};

static int flag;

#define err_return(msg) \
	do { fprintf(stderr, "error at %s\n",msg); free(((struct task*)_task)->outFile); free(_task); close(soc); flag=1; pthread_exit(NULL); } while(0)

static void* doGCP(void* _task){
	
	char* fname = ((struct task*)_task)->fname;
	int wnum = ((struct task*)_task)->wnum;

	int soc = socket(PF_INET, SOCK_STREAM, 0);
	
    {
    	int sz = sizeof(struct sockaddr_in);
    	struct sockaddr_in fadd;
    	fadd.sin_family = AF_INET;
    	fadd.sin_port = Adds[wnum].dport;
    	fadd.sin_addr = Adds[wnum].ip;

    	int rt = connect(soc, aCast(&fadd), sz);
    	if(rt < 0) err_return("doGCP:connect");

		stamp3(" connected to ", &fadd);

		int val = 0;
	    ioctl(soc, FIONBIO, &val);

	    rt = write(soc, "GCP", 4);
	    if(rt < 0) exit_on_error("doGCP:write:GCP");  // rt = 0 means connection closed from the other side
    	if(rt == 0) err_return("doGCP:write:GCP");
    	

    	char buf[BUFSIZ];

    	rt = read(soc, buf, BUFSIZ);
	    if(rt < 0) exit_on_error("doGCP:read:OK");
    	if(rt == 0) err_return("doGCP:read:OK");
    	
	    rt = write(soc, fname, strlen(fname)+1);
	    if(rt < 0) exit_on_error("doGCP:write:fname");
    	if(rt == 0) err_return("doGCP:write:fname");

    	stamp2(" issued instructions to ", soc, 1);

    	int out = open(((struct task*)_task)->outFile, O_CREAT | O_RDWR | O_TRUNC, 0666);
		if(out < 0) exit_on_error("doGCP:open");

		rt = write(soc, "OK", 3);
	    if(rt < 0) exit_on_error("doGCP:write:OK1");
    	if(rt == 0) err_return("doGCP:write:OK1");
    	
    	while(1){
    		char msg[3];
    		rt = read(soc, msg, 3);
    		if(rt < 0) exit_on_error("doGCP:read");
    		if(rt == 0) err_return("doGCP:read");
    		
    		if(strncasecmp(msg,"KK",2)==0){ close(out); break; }

    		rt = write(out, msg, (rt<3 ? rt: 3));
	    	if(rt < 0) exit_on_error("doGCP:write:outFile");
    	}
		rt = write(soc, "OK", 3);
	    if(rt < 0) exit_on_error("doGCP:write:OK2");
    	if(rt == 0) err_return("doGCP:write:OK2");	
		close(out);
	
	}

	stamp2(" received response from ", soc, 1);
	close(soc);
	free(((struct task*)_task)->outFile);
	free(_task);

	return NULL;
}

void* doTask(void* _taskDetails){
	if(pthread_detach(pthread_self())!=0) exit_on_error("doTask:pthread_detach");

	struct taskD* x = (struct taskD*)_taskDetails;
	int candidate[x->nfiles];
	int allocations[nWorkers];
	int i, j;
	for(i=0;i<x->nfiles;++i) candidate[i] = 0;
	for(i=0;i<nWorkers;++i) allocations[i] = 0;
	lock(&fl);
	for(i=0;i<x->nfiles;++i){
		j = 0;
		while(j < fsize && fileList[j].name != NULL){
			if(strcasecmp(fileList[j].name, x->files[i])==0){
				int k, min = 0;
				for(k=0;k<10;++k){
					if(fileList[j].indexes[k]==0) break;
					if(Adds[fileList[j].indexes[k]].cport && allocations[fileList[j].indexes[k]] < allocations[fileList[j].indexes[min]])
						min = k;
				}
			 ++allocations[fileList[j].indexes[min]];
			 candidate[i] = fileList[j].indexes[min];
			}
			++j;
		}
	}
	unlock(&fl);

	for(i=0;i<x->nfiles;++i)
		if(candidate[i]==0) break;
	

	if(i!=x->nfiles){
		char msg[50];
		sprintf(msg, " TASK %d killed: unable to locate resources ", x->taskNum);
		stamp(msg);
		printf("TASK %d killed: unable to locate resources\n", x->taskNum);
		fflush(stdout);
	}


	else{

		flag = 0;
		pthread_t threads[x->nfiles];

		for(i=0;i<x->nfiles;++i){
			struct task* tk = malloc(sizeof(struct task));
			tk->wnum = candidate[i];
			tk->fname = x->files[i];
			tk->outFile = malloc(15);
			sprintf(tk->outFile, "OUT_%d_%d", x->taskNum, i);
			if(pthread_create(&threads[i], NULL, doGCP, tk)!=0)  // doGCP is the only functionality we have for now
				exit_err("doTask:pthread_create:non-zero return value");
			char msg[60];
			sprintf(msg, " created task thread for output file %s ", tk->outFile);
			stamp(msg);

		}
		
		for(i=0;i<x->nfiles;++i)
			pthread_join(threads[i], NULL);

		if(flag==1) { 
			char msg[65];
			sprintf(msg, " Task %d could not be completed due to lost connection(s) ", ((struct taskD*)_taskDetails)->taskNum);
			printf(" Task %d could not be completed due to lost connection(s) ", ((struct taskD*)_taskDetails)->taskNum);
			fflush(stdout);
			stamp(msg);
		}

		else{
			char msg[25];
			sprintf(msg, " Task %d completed ", ((struct taskD*)_taskDetails)->taskNum);
			printf("Task %d completed\n", ((struct taskD*)_taskDetails)->taskNum);	
			fflush(stdout);
			stamp(msg);
		}
		
	}
	
	free(x);
	return NULL;
}
#ifndef _DATA_H
#define _DATA_H

struct taskD{
	char** files;
	int nfiles, taskNum;
	char taskCode[4];
};

void* doTask(void*);

#endif
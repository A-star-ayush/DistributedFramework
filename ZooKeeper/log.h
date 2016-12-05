#ifndef _LOG_H
#define _LOG_H

void initStamp(char* fname);
void stamp(char* msg);
void stamp2(char* msg, int soc, int type);
void stamp3(char* msg, void* add);

#endif
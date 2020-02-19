#include<stdlib.h>
#include <time.h>

#ifndef MEMKEEP_H
#define MEMKEEP_H
struct memoria{
	void * puntatore;
	size_t nmemb;
    size_t size;
	time_t alloc;
	time_t dealloc;
    char nome[10];
	int line;
	struct memoria * next;
};


extern struct memoria* testa_memoria;

void * CalloC(size_t nmemb, size_t size, int line, char * nome);
void FreE(void *prt, int line, char* dove);
void MemstatS();

#define myFree(a)   FreE(a,__LINE__,__FILE__)
#define myCalloc(a,b)  CalloC(a,b,__LINE__,__FILE__);

#endif

#include<stdlib.h>
#include<time.h>
#include<string.h>
#include<stdio.h>
#include "memkeep.h"

struct memoria* testa_memoria=NULL;


/*
 * come calloc, ma salva in una struttura
 * indi rizzo,tipo, ora di allocazione,/deallocazione
 * nome
 */

void * CalloC(size_t nmemb, size_t size, int line, char * nome){

	struct memoria * memp;
	struct memoria * old;
	void * pointer;


	memp=(struct memoria *)calloc(1,sizeof(struct memoria));

	pointer=calloc(nmemb,size);

	memp->puntatore=pointer;
 	memp->size=size;
	memp->nmemb=nmemb;
	memp->alloc=time(NULL);
	memp->dealloc=0;
	strncpy(memp->nome,nome,9);
	memp->line=line;
	
	old=testa_memoria;
	testa_memoria=memp;
	testa_memoria->next=old;
	return pointer;
}


void FreE(void *prt, int line, char* dove){
	struct memoria * p;

	for(p=testa_memoria;p;p=p->next){
			if (p->puntatore==prt)
					break;
	};
	if (p==NULL)
			printf("%s, %d. Never allocated!",dove, line);
		 else {
		if (p->dealloc!=0)
				printf("%s, %d. Already dealloc (for where...next release!) ",dove, line);
		else 
				p->dealloc=time(NULL);
		 }
	free(prt);
	return;
}
	
void MemstatS(){
	struct memoria * p;

	for(p=testa_memoria;p;p=p->next){
			if (p->dealloc!=0){
					continue;
			};
			printf("not deall:%ld, %s, %d\n",p->alloc,p->nome, p->line);
	}

	return;
}

/*
 *  Copyright (C) 2004 by Marco Caldarelli and Riccardo Vianello
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 */
/****************************************************************************
* Cittadella/UX share                    (C) M. Caldarelli and R. Vianello  *
*                                                                           *
* File : prefixtree.c                                                       *
*        Library to build and use a prefix tree                             *
* Author : Tommaso Schiavinotto                                             *
****************************************************************************/
#include <stdlib.h>
#include <string.h>
#include"prefixtree.h"

#define USE_MEM_STAT  1 /* TODO sistemare */

#include "macro.h"



static int rlAddReference(ptree_t *pt, reflist_t **rfl, reference_t *rf, int sort) {
    reflist_t *r, *v, *srfl;
    
    /* Head of the list or not sorting */
    if (!rfl) {
	pt->error = PT_ERR;
	return(0);
    }
    srfl = *rfl;
    /* the inserted value is the same of the head, do nothing */
    if (srfl && pt->refCmp(rf->data, srfl->ref->data) == 0) return(0);
    
    if (!sort || !srfl || (pt->refCmp(rf->data, srfl->ref->data) < 0)) {
      CREATE(r, reflist_t, 1, TYPE_REFLIST);
      /* 
	if ( !(r=(reflist_t *)malloc(sizeof(reflist_t))) ) {
	  pt->error = PT_ERR;
	  return(0);
	}
      */
	r->ref = rf;
	r->next = srfl;
	rf->counter++;
	*rfl = r; /* The only case rfl is modified is when the head is changed */
	return(1);
    }

    for (v = srfl; (v->next) && (pt->refCmp(v->next->ref->data, rf->data) < 0); v=v->next);

    if ( (!v->next) || (pt->refCmp(v->next->ref->data, rf->data)) ){
	reflist_t *vp;
	CREATE(r, reflist_t, 1, TYPE_REFLIST);
	/*
	if ( !(r=(reflist_t *)malloc(sizeof(reflist_t))) ) {
	  pt->error = PT_ERR;
	    return(0);
	}
	*/
	vp = v->next;
	v->next = r;
	r->ref = rf;
	r->next = vp;
	rf->counter++;
	return(1);
    } /* else -> already existing with that reference */

    return(0);
}



static ptnode_t *ptLabel(ptnode_t *m, char lbl) {
    int i;
    ptnode_t *r;
    
    for( i = 0; (i < m->nkids) && (m->kids[i].label != lbl); i++ );
    if (i == m->nkids) {
	if (m->nkids == PT_MAX_KIDS) {
	  return(NULL);
	}
	if (m->nkids) 
	  r=(ptnode_t *)Realloc(m->kids,(m->nkids+1)*sizeof(ptnode_t));
	  /* r=(ptnode_t *)realloc(m->kids,(m->nkids+1)*sizeof(ptnode_t)); */
	else 
	  CREATE(r, ptnode_t, 1, TYPE_PTNODE);
	/*  r=(ptnode_t *)malloc(sizeof(ptnode_t)); */

	if (!r) return(NULL);

	m->kids=r;
	m->nkids++;

	r = &(m->kids[m->nkids - 1]);
	r->label = lbl;
	r->reflist = NULL;
	r->nkids = 0;
	r->kids = NULL;
    } else 
	r = &(m->kids[i]);

    return(r);
}

static ptnode_t *ptSeekLabel(ptnode_t *m, char lbl) {
    int i;
    ptnode_t *r;
    
    for( i = 0; (i < m->nkids) && (m->kids[i].label != lbl); i++ );
    if (i == m->nkids) 
	r=NULL;
    else 
	r=&(m->kids[i]);
    
    return(r);
}

static int ptotAddWord(ptree_t *pt, ptnode_t *t, 
		       const char *w, reference_t *ref) {
    ptnode_t *r;
    int v;

    if (!t) return(-1);    /* The tree cannot be empty  */
    if (!w[0]) return(-1); /* This can happen only with an empty string */

    if ( !(r=ptLabel(t, w[0])) ) {
      pt->error = PT_ERR;
      return(0);
    }
        
    if ( !w[1] ) {  /* The word is at the end */
	v = rlAddReference(pt, &(r->reflist), ref, 0);
    }
    else {
	v = ptotAddWord(pt, r, &w[1], ref);
    }

    return(v);
}



static reflist_t *ptotLook4Word(ptree_t *pt, ptnode_t *t, 
				const char *w, int exact) {
    reflist_t *r;
    
    if (!t) return(NULL); /* the word has not been found */

    if (w[0]) 
	r = ptotLook4Word(pt, ptSeekLabel(t, w[0]), &w[1], exact);
    else { 
	reflist_t *v, *l;
	int i;

	r = NULL;
	if (t->reflist) {
	    for (i = 0, v = t->reflist; v; v = v->next, i++) {
		rlAddReference(pt, &r, v->ref, 1);
	    }
	    /* printf("Found %d References.\n", i); */
	} 
	if (!exact)
	    for (i = 0; i < t->nkids; i++) {
		if ( (v = ptotLook4Word(pt, &(t->kids[i]), w, exact)) ) {
		  /* reflist_t *lp; */

		  /* lp=NULL; */
		    for (l = v; l; l = rlNext(l)) 
			rlAddReference(pt, &r, l->ref, 1);
		} 
	}
    }
    return(r);
}

static void refFree(reference_t *r) {

  r->counter--;

  if ( r->counter <= 0 ) {
    Free(r->data);
    Free(r);
  }
    
}


static void rlDeallocate(ptree_t *pt, reflist_t *v) {
  for (; v; v = rlNext(v));
}


static void ptotDeallocate(ptree_t *pt, ptnode_t *t) {
    int i;

    if (t->reflist) rlDeallocate(pt, t->reflist);

    for (i = 0; i < t->nkids; i++)
	ptotDeallocate(pt, &(t->kids[i]));
    Free(t->kids);
}


/* INTERFACE */

int _rfDefCmp(void *a, void *b) {
    return(-1);
}


ptree_t *ptCreate(int (*rfcmp)(void *, void *)) {
    ptree_t *t;

    CREATE(t, ptree_t, 1, TYPE_PTREE);
    /* t=(ptree_t *)malloc(sizeof(ptree_t)); */

    if (!t) return(NULL);

    t->error = PT_OK;

    t->tree->label=0;
    t->tree->reflist=NULL;
    t->tree->nkids=0;
    t->tree->kids=NULL;
    t->refCmp   = rfcmp?rfcmp:_rfDefCmp;

    return(t);

}



reference_t *refCreate(void *data, size_t n) {
  reference_t *r;
  
  CREATE(r, reference_t, 1, TYPE_PTREF);
  /*r = (reference_t *)malloc(sizeof(reference_t));*/
  CREATE(r->data, void *, n, TYPE_VOIDSTAR);
  /*r->data = (void *)malloc(n);*/ /* (*) Qui CREATE non puo' essere usato?? */
  memcpy(r->data, data, n);
  r->counter = 0;
  return(r);
}


int  refClose(reference_t *r) {
  int k;
  
  k = r->counter; 

  if ( k ) return(k);

  Free(r->data);
  Free(r);
  return(0);
}

int rlLength(reflist_t *rl) {
  int n = 0;
  reflist_t *rle;

  for (rle = rl; rle; rle = rle->next) n++;

  return(n);
}

reflist_t *rlNext(reflist_t *rl) {
  reflist_t *r;

  if (!rl) return(NULL);

  r = rl->next;
  
  refFree(rl->ref);
  Free(rl);

  return(r);
}


int ptAddWord(ptree_t *pt, const char *w, reference_t *ref) {
    return(ptotAddWord(pt, pt->tree, w, ref));
}

reflist_t *ptLook4Word(ptree_t *pt, const char *w, int exact) {
    return(ptotLook4Word(pt, pt->tree, w, exact));
}

void ptDeallocate(ptree_t *pt) {
    ptotDeallocate(pt, pt->tree);
}

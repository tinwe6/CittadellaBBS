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
* File : prefixtree.h                                                       *
*        Library to build and use a prefix tree                             *
* Author : Tommaso Schiavinotto                                             *
****************************************************************************/
#ifndef _PREFIXTREE_H_
#define _PREFIXTREE_H_

#define PT_MAX_KIDS 128

#define PT_OK  0
#define PT_ERR 1


typedef struct reference {
  void *data;
  int counter;
} reference_t;

typedef struct reflist {
    reference_t *ref;
    struct reflist *next;
} reflist_t;

typedef struct ptnode {
    char label;
    reflist_t *reflist;
    unsigned char nkids;
    struct ptnode *kids;
} ptnode_t;

typedef struct ptree {
    ptnode_t tree[1];
    int error;
    int (*refCmp)(void *, void *);
    struct ptnode *kids;
} ptree_t;


ptree_t *ptCreate(int (*rfcmp)(void *, void *));

reference_t *refCreate(void *data, size_t n);
int  refClose(reference_t *r);

reflist_t *rlNext(reflist_t *rl);
int rlLength(reflist_t *rl);

#define refData(r) (r)->data

int ptAddWord(ptree_t *, const char *w, reference_t *);
reflist_t *ptLook4Word(ptree_t *, const char *, int);
void ptDeallocate(ptree_t *);

#endif /* _PREFIXTREE_H_ */

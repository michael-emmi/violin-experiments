/******************************************************************************
 * ptst.c
 * 
 * Per-thread state management. Essentially the state management parts
 * of MB's garbage-collection code have been pulled out and placed here,
 * for the use of other utility routines.
 * 
 * Copyright (c) 2002-2003, K A Fraser
 * 
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
// #include "portable_defns_true.h"
#include "ptst.h"

#define WMB() __asm__ __volatile__ ("" : : : "memory")
#define WMB_NEAR_CAS() WMB()

#define CACHE_LINE_SIZE 64
#define ALIGNED_ALLOC(_s)                                       \
    ((void *)(((unsigned long)malloc((_s)+CACHE_LINE_SIZE*2) +  \
        CACHE_LINE_SIZE - 1) & ~(CACHE_LINE_SIZE-1)))

//### CODE MODIFICATION replaced pthread functions by that:
void *pthread_getspecific(pthread_key_t key) {
    return NULL;
}

int pthread_setspecific(pthread_key_t key, const void *value) {
    return 0;
}

int pthread_key_create(pthread_key_t *key, void (*destructor)(void*)) {
    return 0;
}

int __VERIFIER_atomic_CASIO(int* ptr,int old,int newv) {
    int loaded = *ptr;
    if (loaded == old)
        *ptr = newv;
    return loaded;
}

void* __VERIFIER_atomic_FASPO(void** ptr,void* newv) {
    void* loaded = *ptr;
    *ptr = newv;
    return loaded;
}

void* __VERIFIER_atomic_CASPO(void** ptr,void* old,void* newv) {
    void* loaded = *ptr;
    if (loaded == old)
        *ptr = newv;
    return loaded;
}

pthread_key_t ptst_key;
ptst_t *ptst_list;

static unsigned int next_id;

ptst_t *critical_enter(void)
{
    ptst_t *ptst, *next, *new_next;
    unsigned int id, oid;

    ptst = (ptst_t *)pthread_getspecific(ptst_key);
    if ( ptst == NULL ) 
    {
        for ( ptst = ptst_first(); ptst != NULL; ptst = ptst_next(ptst) ) 
        {
            if ( (ptst->count == 0) && (__VERIFIER_atomic_CASIO(&ptst->count, 0, 1) == 0) ) 
            {
                break;
            }
        }
        
        if ( ptst == NULL ) 
        {
            ptst = ALIGNED_ALLOC(sizeof(*ptst));
            if ( ptst == NULL ) exit(1);
            memset(ptst, 0, sizeof(*ptst));
//             ptst->gc = gc_init();
//             rand_init(ptst);
            ptst->count = 1;
            id = next_id;
            while ( (oid = __VERIFIER_atomic_CASIO(&next_id, id, id+1)) != id ) id = oid;
            ptst->id = id;
            new_next = ptst_list;
            do {
                ptst->next = next = new_next;
                WMB_NEAR_CAS();
            } 
            while ( (new_next = __VERIFIER_atomic_CASPO(&ptst_list, next, ptst)) != next );
        }
        
        pthread_setspecific(ptst_key, ptst);
    }
    
//     gc_enter(ptst);
    return(ptst);
}


static void ptst_destructor(ptst_t *ptst) 
{
    ptst->count = 0;
}


void _init_ptst_subsystem(void) 
{
    ptst_list = NULL;
    next_id   = 0;
    WMB();
//     if ( pthread_key_create(&ptst_key, (void (*)(void *))ptst_destructor) )
//     {
//         exit(1);
//     }
}

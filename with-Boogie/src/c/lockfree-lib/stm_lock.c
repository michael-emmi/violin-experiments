/******************************************************************************
 * stm_lock.c
 * 
 * Lock-based software transactional memory (STM).
 * Uses two-phase locking with multi-reader locks.
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

#include "portable_defns.h"
#include "ptst.h"
#include "gc.h"
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <setjmp.h>
#include <signal.h>

typedef struct stm_blk_st stm_blk;
typedef struct stm_tx_entry_st stm_tx_entry;
typedef struct stm_tx_st stm_tx;
typedef struct stm_st stm;

struct stm_blk_st {
    void *data;
    mrsw_lock_t lock;
};

struct stm_tx_entry_st {
    stm_blk *b;
    void    *old;
    void    *new;
    stm_tx_entry *next;
};

struct stm_tx_st {
    int status;
    stm_tx_entry *blocks;
    stm_tx_entry *alloc_ptr, *check;
    int gc_data_id, blk_size; /* copied from 'stm' structure */
    sigjmp_buf *penv;
};

struct stm_st {
    int gc_data_id;
    int blk_size;
};

#define DESCRIPTOR_IN_USE(_t) ((_t)->penv != NULL)

#define DESCRIPTOR_SIZE  4096
#define MAX_TX_ENTS      (DESCRIPTOR_SIZE / sizeof(stm_tx_entry))

/* Private per-thread state. The array is indexed off ptst->id. */
typedef struct {
    char desc[DESCRIPTOR_SIZE];
} priv_t;

static priv_t priv_ptst[MAX_THREADS];
static int gc_blk_id;  /* Allocation id for block descriptors. */
static int do_padding; /* Should all allocations be padded to a cache line? */

#define ALLOCATOR_SIZE(_s) (do_padding ? CACHE_LINE_SIZE : (_s))

#define TXS_IN_PROGRESS 0
#define TXS_FAILED      1
#define TXS_SUCCESSFUL  2

bool_t commit_stm_tx(ptst_t *ptst, stm_tx *t);

static stm_tx_entry *alloc_stm_tx_entry(stm_tx *t)
{
    stm_tx_entry *ent = t->alloc_ptr++;
    assert(((unsigned long)t->alloc_ptr - (unsigned long)t) <=
           DESCRIPTOR_SIZE);
    return ent;
}


static stm_tx_entry **search_stm_tx_entry(stm_tx_entry **pnext, stm_blk *b)
{
    stm_tx_entry *next = *pnext;

    while ( (next != NULL) && ((unsigned long)next->b < (unsigned long)b) )
    {
        pnext = &next->next;
        next  = *pnext;
    }

    return pnext;
}


stm *new_stm(ptst_t *ptst, int blk_size)
{
    stm *mem = malloc(CACHE_LINE_SIZE);
    mem->blk_size = blk_size;
    mem->gc_data_id = gc_add_allocator(ALLOCATOR_SIZE(blk_size));
    return mem;
}


void free_stm(ptst_t *ptst, stm *mem)
{
    gc_remove_allocator(mem->gc_data_id);
    free(mem);
}


stm_blk *new_stm_blk(ptst_t *ptst, stm *mem)
{
    stm_blk *b;
    b       = gc_alloc(ptst, gc_blk_id);
    b->data = gc_alloc(ptst, mem->gc_data_id);
    mrsw_init(&b->lock);
    return b;
}


void free_stm_blk(ptst_t *ptst, stm *mem, stm_blk *b)
{
    gc_free(ptst, b->data, mem->gc_data_id);
    gc_free(ptst, b,       gc_blk_id);
}


void *init_stm_blk(ptst_t *ptst, stm *mem, stm_blk *b)
{
    return b->data;
}


int sizeof_stm_blk(ptst_t *ptst, stm *mem, stm_blk *b)
{
    return mem->blk_size;
}


stm_tx *new_stm_tx(ptst_t *ptst, stm *mem, sigjmp_buf *penv)
{
    stm_tx *t = (stm_tx *)&priv_ptst[ptst->id];
    if ( DESCRIPTOR_IN_USE(t) ) goto nesting;
    t->status = TXS_IN_PROGRESS;
    t->blocks = NULL;
    t->alloc_ptr = t->check = (stm_tx_entry *)(t + 1);
    t->gc_data_id = mem->gc_data_id;
    t->blk_size = mem->blk_size;
    t->penv = penv;
    return t;

 nesting:
    fprintf(stderr, "No nesting of transactions is allowed\n");
    return NULL;
}


bool_t commit_stm_tx(ptst_t *ptst, stm_tx *t)
{
    stm_tx_entry *ent, *last_ent;
    mrsw_qnode_t qn[MAX_TX_ENTS];
    stm_blk *b;
    void *old;
    int i;

    t->penv = NULL;

    /* Outcome may have been decided by an 'abort' or 'validate' operation. */
    if ( t->status != TXS_IN_PROGRESS ) goto out;

    /* We start by taking locks in order, and checking old values. */
    for ( i = 0, ent = t->blocks; ent != NULL; i++, ent = ent->next )
    {
        b = ent->b;
        if ( (old = ent->old) == ent->new )
        {
            rd_lock(&b->lock, &qn[i]);
        }
        else
        {
            wr_lock(&b->lock, &qn[i]);
        }
        /* Check old value, and shortcut to failure if we mismatch. */
        if ( b->data != old ) goto fail;
    }

    /*
     * LINEARISATION POINT FOR SUCCESS:
     * We haven't written new values yet, but that's okay as we have write
     * locks on those locations. Noone can see old value now and yet still
     * commit (as they'll be waiting for the read lock).
     */
    t->status = TXS_SUCCESSFUL;

    /* We definitely succeed now: release locks and write new values. */
    for ( i = 0, ent = t->blocks; ent != NULL; i++, ent = ent->next )
    {
        b = ent->b;
        if ( ent->old == ent->new )
        {
            rd_unlock(&b->lock, &qn[i]);
        }
        else
        {
            b->data = ent->new;
            wr_unlock(&b->lock, &qn[i]);
        }
    }

 out:
    if ( t->status == TXS_SUCCESSFUL )
    {
        for ( ent = t->blocks; ent != NULL; ent = ent->next )
        {
            if ( ent->old == ent->new ) continue;
            gc_free(ptst, ent->old, t->gc_data_id);
        }
        return TRUE;
    }
    else
    {
        for ( ent = t->blocks; ent != NULL; ent = ent->next )
        {
            if ( ent->old == ent->new ) continue;
            gc_unsafe_free(ptst, ent->new, t->gc_data_id);
        }
        return FALSE;
    }    

    /*
     * We put (hopefully rare) failure case out-of-line here.
     * This is also the LINEARISTAION POINT FOR FAILURE.
     */
 fail:
    last_ent = ent->next;
    t->status = TXS_FAILED;
    for ( i = 0, ent = t->blocks; ent != last_ent; i++, ent = ent->next )
    {
        b = ent->b;
        if ( ent->old == ent->new )
        {
            rd_unlock(&b->lock, &qn[i]);
        }
        else
        {
            wr_unlock(&b->lock, &qn[i]);
        }
    }
    goto out;
}


bool_t validate_stm_tx(ptst_t *ptst, stm_tx *t)
{
    stm_tx_entry *ent, *last_ent = NULL;
    mrsw_qnode_t qn[MAX_TX_ENTS];
    stm_blk *b;
    void *old;
    int i;

    RMB();

    /* Lock-acquire phase */
    for ( i = 0, ent = t->blocks; ent != NULL; i++, ent = ent->next )
    {
        b = ent->b;

        if ( (old = ent->old) == ent->new )
        {
            rd_lock(&b->lock, &qn[i]);
        }
        else
        {
            wr_lock(&b->lock, &qn[i]);
        }

        if ( b->data != old )
        {
            t->status = TXS_FAILED;
            last_ent = ent->next;
            break;
        }
    }

    /* Lock-release phase */
    for ( i = 0, ent = t->blocks; ent != last_ent; i++, ent = ent->next )
    {
        b = ent->b;
        if ( ent->old == ent->new )
        {
            rd_unlock(&b->lock, &qn[i]);
        }
        else
        {
            wr_unlock(&b->lock, &qn[i]);
        }
    }

    return t->status != TXS_FAILED;
}


void abort_stm_tx(ptst_t *ptst, stm_tx *t)
{
    t->status = TXS_FAILED;
}


void *read_stm_blk(ptst_t *ptst, stm_tx *t, stm_blk *b)
{
    stm_tx_entry **pent, *ent;
    sigjmp_buf *penv;
    void *result;

    pent = search_stm_tx_entry(&t->blocks, b);
    ent  = *pent;
    if ( (ent != NULL) && (ent->b == b) ) goto found;

    ent = alloc_stm_tx_entry(t);
    ent->b    = b;
    ent->old  = b->data;
    ent->new  = ent->old;
    ent->next = *pent;
    *pent = ent;
    return ent->new;

 found:
    result = ent->new;
    ent = t->check;
    if ( ent->b->data != ent->old ) goto fail;
    if ( ++t->check == t->alloc_ptr ) t->check = (stm_tx_entry *)(t + 1);
    return result;

 fail:
    penv = t->penv;
    abort_stm_tx(ptst, t);
    commit_stm_tx(ptst, t);
    siglongjmp(*penv, 0);
    assert(0);
    return NULL;
}


void *write_stm_blk(ptst_t *ptst, stm_tx *t, stm_blk *b)
{
    stm_tx_entry **pent, *ent;
    sigjmp_buf *penv;
    void *result;

    pent = search_stm_tx_entry(&t->blocks, b);
    ent = *pent;
    if ( (ent != NULL) && (ent->b == b) )
    {
        if ( ent->old != ent->new ) goto found;
    }
    else
    {
        ent = alloc_stm_tx_entry(t);
        ent->b    = b;
        ent->old  = b->data;
        ent->next = *pent;
        *pent = ent;
    }

    ent->new  = gc_alloc(ptst, t->gc_data_id);
    memcpy(ent->new, ent->old, t->blk_size);
    return ent->new;

 found:
    result = ent->new;
    ent = t->check;
    if ( ent->b->data != ent->old ) goto fail;
    if ( ++t->check == t->alloc_ptr ) t->check = (stm_tx_entry *)(t + 1);
    return result;

 fail:
    penv = t->penv;
    abort_stm_tx(ptst, t);
    commit_stm_tx(ptst, t);
    siglongjmp(*penv, 0);
    assert(0);
    return NULL;
}


void remove_from_tx(ptst_t *ptst, stm_tx *t, stm_blk *b)
{
    stm_tx_entry **pent, *ent;
    void *data;

    pent = search_stm_tx_entry(&t->blocks, b);
    ent  = *pent;
    if ( (ent != NULL) && (ent->b == b) ) 
    { 
        *pent = ent->next;
        if ( (data = ent->new) != ent->old )
        {
            gc_free(ptst, data, t->gc_data_id);
        }
    }
}


static void handle_fault(int sig)
{
    ptst_t *ptst;
    stm_tx *t;

    ptst = critical_enter();
    t = (stm_tx *)&priv_ptst[ptst->id];
    if ( DESCRIPTOR_IN_USE(t) && !validate_stm_tx(ptst, t) )
    {
        sigjmp_buf *penv = t->penv;
        commit_stm_tx(ptst, t);
        critical_exit(ptst);
        siglongjmp(*penv, 0);
    }

 fail:
    fprintf(stderr, "Error: unhandleable SIGSEGV!\n");
    abort();
}


void _init_stm_subsystem(int pad_data)
{
    struct sigaction act;

    do_padding = pad_data;
    gc_blk_id = gc_add_allocator(ALLOCATOR_SIZE(sizeof(stm_blk)));
    memset(priv_ptst, 0, sizeof(priv_ptst));

    act.sa_handler = handle_fault;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;
    sigaction(SIGSEGV, &act, NULL);
}

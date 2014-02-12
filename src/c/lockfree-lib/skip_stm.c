/******************************************************************************
 * skip_stm.c
 * 
 * Skip lists, allowing concurrent update by use of the STM abstraction.
 * 
 * Copyright (c) 2003, K A Fraser
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

#define __SET_IMPLEMENTATION__

#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "portable_defns.h"
#include "gc.h"
#include "stm.h"
#include "set.h"

typedef struct node_st node_t;
typedef stm_blk set_t;

struct node_st
{
    int       level;
    setkey_t  k;
    setval_t  v;
    stm_blk  *next[NUM_LEVELS];
};

static struct {
    CACHE_PAD(0);
    stm *memory;    /* read-only */
    CACHE_PAD(2);
} shared;

#define MEMORY (shared.memory)

/*
 * Random level generator. Drop-off rate is 0.5 per level.
 * Returns value 1 <= level <= NUM_LEVELS.
 */
static int get_level(ptst_t *ptst)
{
    unsigned long r = rand_next(ptst);
    int l = 1;
    r = (r >> 4) & ((1 << (NUM_LEVELS-1)) - 1);
    while ( (r & 1) ) { l++; r >>= 1; }
    return l;
}


/*
 * Search for first non-deleted node, N, with key >= @k at each level in @l.
 * RETURN VALUES:
 *  Array @pa: @pa[i] is non-deleted predecessor of N at level i
 *  Array @na: @na[i] is N itself, which should be pointed at by @pa[i]
 *  MAIN RETURN VALUE: same as @na[0], direct pointer open for reading.
 */
static node_t *search_predecessors(
    ptst_t *ptst, stm_tx *tx, set_t *l, setkey_t k, stm_blk **pa, stm_blk **na)
{
    stm_blk *xb, *x_nextb;
    node_t  *x,  *x_next;
    int      i;

    xb = l;
    x  = read_stm_blk(ptst, tx, l);
    for ( i = NUM_LEVELS - 1; i >= 0; i-- )
    {
        for ( ; ; )
        {
            x_nextb = x->next[i];
            x_next  = read_stm_blk(ptst, tx, x_nextb);
            if ( x_next->k >= k ) break;
            xb = x_nextb;
            x  = x_next;
        }

        if ( pa ) pa[i] = xb;
        if ( na ) na[i] = x_nextb;
    }

    return x_next;
}


/*
 * PUBLIC FUNCTIONS
 */

set_t *set_alloc(void)
{
    ptst_t  *ptst;
    stm_blk *hb, *tb;
    node_t  *h, *t;
    int      i;

    ptst = critical_enter();

    tb = new_stm_blk(ptst, MEMORY);
    t  = init_stm_blk(ptst, MEMORY, tb);
    memset(t, 0, sizeof(*t));
    t->k = SENTINEL_KEYMAX;

    hb = new_stm_blk(ptst, MEMORY);
    h  = init_stm_blk(ptst, MEMORY, hb);
    memset(h, 0, sizeof(*h));
    h->k     = SENTINEL_KEYMIN;
    h->level = NUM_LEVELS;
    for ( i = 0; i < NUM_LEVELS; i++ )
        h->next[i] = tb;

    critical_exit(ptst);

    return hb;
}


setval_t set_update(set_t *l, setkey_t k, setval_t v, int overwrite)
{
    ptst_t   *ptst;
    stm_tx   *tx;
    setval_t  ov;
    stm_blk  *bpreds[NUM_LEVELS], *bsuccs[NUM_LEVELS], *newb = NULL;
    node_t   *x, *p, *new;
    int       i;

    k = CALLER_TO_INTERNAL_KEY(k);

    ptst = critical_enter();

    do {
        new_stm_tx(tx, ptst, MEMORY);
        x = search_predecessors(ptst, tx, l, k, bpreds, bsuccs);

        if ( x->k == k )
        {
            x    = write_stm_blk(ptst, tx, bsuccs[0]);
            ov   = x->v;
            x->v = v;
        }
        else
        {
            ov = NULL;

            if ( newb == NULL )
            {
                newb = new_stm_blk(ptst, MEMORY);
                new  = init_stm_blk(ptst, MEMORY, newb);
                new->k = k;
                new->v = v;
                new->level = get_level(ptst);
            }

            for ( i = 0; i < new->level; i++ )
            {
                new->next[i] = bsuccs[i];
                p = write_stm_blk(ptst, tx, bpreds[i]);
                p->next[i] = newb;
            }
        }
    }
    while ( !commit_stm_tx(ptst, tx) );

    if ( (ov != NULL) && (newb != NULL) ) 
        free_stm_blk(ptst, MEMORY, newb);

    critical_exit(ptst);

    return ov;
}


setval_t set_remove(set_t *l, setkey_t k)
{
    setval_t  v;
    ptst_t   *ptst;
    stm_tx   *tx;
    stm_blk  *bpreds[NUM_LEVELS], *bsuccs[NUM_LEVELS];
    node_t   *p, *x;
    int       i;

    k = CALLER_TO_INTERNAL_KEY(k);
  
    ptst = critical_enter();
    
    do {
        new_stm_tx(tx, ptst, MEMORY);
        x = search_predecessors(ptst, tx, l, k, bpreds, bsuccs);
        if ( x->k == k )
        {
            v = x->v;
            for ( i = 0; i < x->level; i++ )
            {
                p = write_stm_blk(ptst, tx, bpreds[i]);
                p->next[i] = x->next[i];
            }
        }
        else
        {
            v = NULL;
        }
    }
    while ( !commit_stm_tx(ptst, tx) );

    if ( v != NULL ) 
        free_stm_blk(ptst, MEMORY, bsuccs[0]);

    critical_exit(ptst);

    return v;
}


setval_t set_lookup(set_t *l, setkey_t k)
{
    setval_t  v;
    ptst_t   *ptst;
    stm_tx   *tx;
    node_t   *x;

    k = CALLER_TO_INTERNAL_KEY(k);

    ptst = critical_enter();

    do {
        new_stm_tx(tx, ptst, MEMORY);
        x = search_predecessors(ptst, tx, l, k, NULL, NULL);
        v = (x->k == k) ? x->v : NULL;
    }
    while ( !commit_stm_tx(ptst, tx) );

    critical_exit(ptst);

    return v;
}


void _init_set_subsystem(void)
{
    ptst_t *ptst = critical_enter();
    _init_stm_subsystem(0);
    MEMORY = new_stm(ptst, sizeof(node_t));
    critical_exit(ptst);
}

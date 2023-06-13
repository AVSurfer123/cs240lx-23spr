// engler, cs140e: brain-dead generic queue. don't want to use STL/c++ in the kernel.
#include "rpi.h"
#ifndef __Q_H__
#define __Q_H__
#ifndef E_t
#	error "Client must define the Q datatype <E_t>"
#endif

typedef struct Q {
    E_t *head, *tail;
    unsigned cnt;
} Q_t;

// used for iteration.
static E_t *Q_start(Q_t *q)   { return q->head; }
static E_t *Q_next(E_t *e)      { return e->next; }
static unsigned Q_nelem(Q_t *q) { return q->cnt; }

static int Q_empty(Q_t *q)  {
    if(q->head)
        return 0;
    // if(Q_nelem(q) != 0){
    //     printk("nemel: <%d>", Q_nelem(q));
    // }
    // assert(Q_nelem(q) == 0);
    demand(!q->tail, invalid Q);
    return 1;
}

// remove from front of list.
static E_t *Q_pop(Q_t *q) {
    demand(q, bad input);

    E_t *e = q->head;
    if(!e) {
        assert(Q_empty(q));
        return 0;
    }
    q->cnt--;
    q->head = e->next;
    if(!q->head)
        q->tail = 0;
    return e;
}

// insert at tail. (for FIFO)
static void Q_append(Q_t *q, E_t *e) {
    e->next = 0;
    q->cnt++;
    if(!q->tail)
        q->head = q->tail = e;
    else {
        q->tail->next = e;
        q->tail = e;
    }
}

// insert at head (for LIFO)
static void Q_push(Q_t *q, E_t *e) {
    q->cnt++;
    e->next = q->head;
    q->head = e;
    if(!q->tail)
        q->tail = e;
}

// insert <e_new> after <e>: <e>=<null> means put at head.
static void Q_insert_after(Q_t *q, E_t *e, E_t *e_new) {
    if(!e)
        Q_push(q,e_new);
    else if(q->tail == e)
        Q_append(q,e_new);
    else {
        q->cnt++;
        e_new->next = e->next;
        e->next = e_new;
    }
}
#endif

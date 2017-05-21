//
// Created by Mrz355 on 21.05.17.
//

#include <sys/types.h>
#include <stdlib.h>

struct thread_info {
    int tid;
    int function;  // WRITER or READER
};

struct node {
    struct thread_info info;
    struct node *next;
};

struct fifo {
    struct node *first;
    struct node *last;
};

struct fifo* fifo_init() {
    struct fifo *f = malloc(sizeof(struct fifo));
    f->first = NULL;
    return f;
}

void fifo_push(struct fifo *f, struct thread_info info) {
    struct node *p = malloc(sizeof(struct node));
    p->info = info;
    p->next = NULL;

    if(f->first == NULL) {
        f->first = p;
    } else {
        f->last->next = p;
    }
    f->last = p;
}

struct thread_info fifo_pop(struct fifo *f) {
    struct thread_info res;
    if(f->first == NULL) {
        res.tid = -1;
        return res;
    }
    res = f->first->info;

    struct node *tmp = f->first;
    f->first = f->first->next;
    free(tmp);

    return res;
}

int fifo_empty(struct fifo *f) {
    return f->first == NULL;
}

void fifo_destroy(struct fifo *f) {
    while(f->first != NULL) {
        struct node *tmp = f->first;
        f->first = tmp->next;
        free(tmp);
    }
    free(f);
}
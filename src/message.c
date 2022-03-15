#include "message.h"

#include <stdlib.h>

msg_t *msg_queue_pop_first(msg_queue_t *mq) {
    if (mq->size == 0) {
        return NULL;
    }

    msg_t *msg = mq->first;

    if (mq->size == 1) {
        mq->first = NULL;
        mq->last = NULL;
    }
    else {
        mq->first = msg->next;
    }
    mq->size--;

    return msg;
}

void msg_queue_push_last_safe(msg_queue_t *mq, msg_t *msg) {
    pthread_mutex_lock(mq->lock);
    if (mq->size == 0) {
        mq->first = msg;
        mq->last = msg;
    }
    else {
        mq->last->next = msg;
        mq->last = msg;
    }
    mq->size++;
    pthread_mutex_unlock(mq->lock);
}

void msg_queue_fetch_safe(msg_queue_t *to, msg_queue_t *from) {
    pthread_mutex_lock(from->lock);
    if (from->size == 0) {
        pthread_mutex_unlock(from->lock);
        return;
    }
    if (to->size == 0) {
        to->first = from->first;
        to->size = from->size;
    }
    else {
        to->last->next = from->first;
        to->size += from->size;
    }
    to->last = from->last;
    from->first = NULL;
    from->last = NULL;
    from->size = 0;

    pthread_mutex_unlock(from->lock);
}

void msg_free(msg_t *msg) {
    if (msg->data) {
        free(msg->data);
    }
    free(msg);
}

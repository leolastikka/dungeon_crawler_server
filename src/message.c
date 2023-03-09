#include "message.h"
#include "common.h"

#include <stdlib.h>
#include <string.h>

message_t *message_copy(const message_t *msg) {
    message_t *copy = (message_t *)malloc(sizeof(message_t));
    check_malloc(copy);
    *copy = (message_t) {
        .client_id = msg->client_id,
        .type = msg->type,
        .flags = msg->flags
    };
    if (msg->data_len) {
        char *data = (char *)malloc(msg->data_len);
        strncpy(data, msg->data, msg->data_len);
        copy->data = data;
        copy->data_len = msg->data_len;
    }
    return copy;
}


void message_free(message_t *msg) {
    if (msg->data) {
        free(msg->data);
    }
    free(msg);
}

message_t *message_queue_pop(message_queue_t *mq) {
    if (mq->size == 0) {
        return NULL;
    }

    message_t *msg = mq->first;

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

void message_queue_push_safe(message_queue_t *mq, message_t *msg) {
    pthread_mutex_lock(&mq->lock);
    if (mq->size == 0) {
        mq->first = msg;
        mq->last = msg;
    }
    else {
        mq->last->next = msg;
        mq->last = msg;
    }
    mq->size++;
    pthread_mutex_unlock(&mq->lock);
}

void message_queue_fetch_safe(message_queue_t *to, message_queue_t *from) {
    pthread_mutex_lock(&from->lock);
    if (from->size == 0) {
        pthread_mutex_unlock(&from->lock);
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

    pthread_mutex_unlock(&from->lock);
}

void message_queue_clear(message_queue_t *mq) {
    message_t *msg = mq->first;
    while (msg) {
        message_t *next = msg->next;
        message_free(msg);
        msg = next;
    }
    mq->first = NULL;
    mq->last = NULL;
    mq->size = 0;
}

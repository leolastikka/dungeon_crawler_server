#include <stdint.h>
#include <pthread.h>

#ifndef MESSAGE_H
#define MESSAGE_H

// client message types
static const uint8_t MSG_CLIENT_BROADCAST_ECHO = 50; // ascii digit 2

// server message types
static const uint8_t MSG_SERVER_FULL = 255;

// message flags
static const uint8_t MSG_FLAG_BROADCAST = 0x01;

typedef struct message {
    struct message *next;
    void *data;
    int data_len;
    uint32_t client_id;
    uint8_t type;
    uint8_t flags;
} message_t;

typedef struct {
    message_t *first;
    message_t *last;
    pthread_mutex_t lock;
    int size;
} message_queue_t;

message_t *message_copy(const message_t *msg);
void message_free(message_t *msg);

message_t *message_queue_pop(message_queue_t *mq);
void message_queue_push_safe(message_queue_t *mq, message_t *msg);
void message_queue_fetch_safe(message_queue_t *to, message_queue_t *from);
void message_queue_clear(message_queue_t *mq);


#endif
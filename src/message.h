#include "client.h"

#include <stdint.h>
#include <pthread.h>

#ifndef MESSAGE_H
#define MESSAGE_H

// client message types
static const uint8_t MSG_CLIENT_CONNECT = 1;
static const uint8_t MSG_CLIENT_DISCONNECT = 2;
static const uint8_t MSG_CLIENT_KEEPALIVE = 3;
static const uint8_t MSG_CLIENT_INPUT = 4;

// server message types
static const uint8_t MSG_SERVER_CLIENT = 5;
static const uint8_t MSG_SERVER_ADD = 6;
static const uint8_t MSG_SERVER_REMOVE = 7;
static const uint8_t MSG_SERVER_MOVE = 8;
static const uint8_t MSG_SERVER_ACK = 254;
static const uint8_t MSG_SERVER_FULL = 255;

// message flags
static const uint8_t MSG_FLAG_BROADCAST = 0x01;
// static const uint8_t MSG_FLAG_BROADCAST_OTHERS = 0x03;

// message input bits
static const uint8_t MSG_INPUT_X_POS = 0b00000001;
static const uint8_t MSG_INPUT_X_NEG = 0b00000010;
static const uint8_t MSG_INPUT_Y_POS = 0b00000100;
static const uint8_t MSG_INPUT_Y_NEG = 0b00001000;
static const uint8_t MSG_INPUT_R_POS = 0b00010000;
static const uint8_t MSG_INPUT_R_NEG = 0b00100000;

typedef struct message {
    struct message *next;
    client_t *client;
    void *data;
    uint8_t type;
    uint8_t flags;
} msg_t;

typedef struct {
    int size;
    msg_t *first;
    msg_t *last;
    pthread_mutex_t *lock;
} msg_queue_t;

msg_t *msg_queue_pop_first(msg_queue_t *mq);
void msg_queue_push_last_safe(msg_queue_t *mq, msg_t *msg);
void msg_queue_fetch_safe(msg_queue_t *to, msg_queue_t *from);
void msg_free(msg_t *msg);

#endif
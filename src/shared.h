#include "client.h"
#include "message.h"

#include <stdint.h>

#ifndef SHARED_H
#define SHARED_H

typedef struct game_shared {
    msg_queue_t *mq_in;
    msg_queue_t *mq_out;
    int game_fps;
} game_shared_t;

typedef struct listener_shared {
    msg_queue_t *mq_in;
    msg_queue_t *mq_out;
    client_list_t *clients;
    int socket_fd;
    int buffer_size;
    uint16_t port;
    uint8_t max_clients;
} listener_shared_t;

typedef struct sender_shared {
    msg_queue_t *mq_out;
    client_list_t *clients;
    int socket_fd;
    int buffer_size;
} sender_shared_t;

typedef struct timeout_shared {
    msg_queue_t *mq_in;
    client_list_t *clients;
    uint32_t timeout_sec;
} timeout_shared_t;

#endif
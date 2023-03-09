#include "client.h"
#include "message.h"

#include <pthread.h>
#include <stdint.h>

#ifndef SHARED_H
#define SHARED_H

typedef struct game_shared {
    message_queue_t *in_mq;
    message_queue_t *out_mq;
    client_list_t *clients;
    int game_fps;
} game_shared_t;

typedef struct listener_shared {
    message_queue_t *in_mq;
    message_queue_t *out_mq;
    client_list_t *clients;
    int socket_fd;
    int buffer_size;
    uint16_t port;
    uint8_t max_clients;
    uint8_t max_waiting_connections;
} listener_shared_t;

typedef struct client_shared {
    message_queue_t *in_mq;
    message_queue_t *out_mq;
    client_list_t *clients;
    client_t *client;
    int buffer_size;
} client_shared_t;

#endif
#include "message.h"

#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>

#ifndef CLIENT_H
#define CLIENT_H

typedef struct client {
    struct client *next;
    message_queue_t mq;
    pthread_t thread_id;
    int socket_fd;
    uint32_t id;
    bool close;
} client_t;

typedef struct client_list {
    client_t *first;
    client_t *last;
    pthread_mutex_t lock;
    int size;
    int max_size;
} client_list_t;

bool is_client_list_full_safe(client_list_t *clients);
client_t *client_list_add_safe(client_list_t *clients, client_t *client);
client_t *client_list_find_safe(client_list_t *clients, uint32_t cid);
void client_list_remove(client_list_t *clients, uint32_t cid);
void client_list_remove_safe(client_list_t *clients, uint32_t cid);

#endif
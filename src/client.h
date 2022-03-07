#include "entity.h"

#include <pthread.h>
#include <stdint.h>
#include <time.h>

#ifndef CLIENT_H
#define CLIENT_H

typedef struct client {
    struct client *next;
    uint32_t id;
    uint32_t address;
    uint16_t port;
    time_t last_receive_time;
    entity_t *entity;
} client_t;

typedef struct client_list {
    int size;
    int max_size;
    client_t *first;
    client_t *last;
    pthread_mutex_t *lock;
} client_list_t;

int client_list_full(client_list_t *clients);
client_t *client_list_add_safe(client_list_t *clients, uint32_t cid, uint32_t addr, uint16_t port);
client_t *client_list_find_safe(client_list_t *clients, uint32_t cid);
void client_list_remove(client_list_t *clients, uint32_t cid);
void client_list_remove_safe(client_list_t *clients, uint32_t cid);

#endif
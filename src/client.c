#include "client.h"
#include "common.h"

#include <stdlib.h>

bool is_client_list_full(client_list_t *clients) {
    return clients->size == clients->max_size;
}

bool is_client_list_full_safe(client_list_t *clients) {
    pthread_mutex_lock(&clients->lock);
    bool result = clients->size == clients->max_size;
    pthread_mutex_unlock(&clients->lock);
    return result;
}

client_t *client_list_add_safe(client_list_t *clients, client_t *client) {
    pthread_mutex_lock(&clients->lock);

    if (is_client_list_full(clients)) {
        pthread_mutex_unlock(&clients->lock);
        return NULL;
    }

    if (clients->size == 0) {
        clients->first = client;
        clients->last = client;
    }
    else {
        clients->last->next = client;
        clients->last = client;
    }
    clients->size++;

    pthread_mutex_unlock(&clients->lock);

    return client;
}

client_t *client_list_find_safe(client_list_t *clients, uint32_t cid) {
    pthread_mutex_lock(&clients->lock);

    client_t *client = clients->first;
    while (client) {
        if (client->id == cid) {
            pthread_mutex_unlock(&clients->lock);
            return client;
        }
        else {
            client = client->next;
        }
    }

    pthread_mutex_unlock(&clients->lock);
    return NULL;
}

void client_list_remove(client_list_t *clients, uint32_t cid) {
    client_t *client = clients->first;
    client_t *prev = NULL;
    while (client) {
        if (client->id == cid) { // found cid
            if (clients->size == 1) {
                clients->first = NULL;
                clients->last = NULL;
            }
            else {
                if (prev) { // if not first
                    prev->next = client->next;

                    if (client == clients->last) { // if last
                        clients->last = prev;
                    }
                }
                else { // if first
                    clients->first = client->next;
                }
            }
            clients->size--;
            free(client);
            client = NULL;
            return;
        }
        else {
            prev = client;
            client = client->next;
        }
    }
}

void client_list_remove_safe(client_list_t *clients, uint32_t cid) {
    pthread_mutex_lock(&clients->lock);
    client_list_remove(clients, cid);
    pthread_mutex_unlock(&clients->lock);
}

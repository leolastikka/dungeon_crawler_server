#include "shared.h"

#include <stdio.h>
#include <stdlib.h>

void *game_thread(void *arg) {
    game_shared_t *shared = (game_shared_t*)arg;
    message_queue_t local_mq = {0};

    printf("Game thread running\n");
    while (1) {
        // fetch incoming messages
        message_queue_fetch_safe(&local_mq, shared->in_mq);
        if (local_mq.size != 0) {
            message_t *msg = message_queue_pop(&local_mq);
            while (msg) {
                printf("Game processing message type %d\n", msg->type);

                uint8_t type = msg->type;
                uint32_t client_id = msg->client_id;

                message_free(msg);
                msg = message_queue_pop(&local_mq);

                // now just echo messages back to clients
                message_t *msg_echo = (message_t *)malloc(sizeof(message_t));
                *msg_echo = (message_t) {
                    .type = type,
                    .client_id = client_id
                };

                // broadcast msg type 50 (ascii digit 2) for testing
                if (type == MSG_CLIENT_BROADCAST_ECHO) {
                    msg_echo->flags = MSG_FLAG_BROADCAST;
                }

                // send echo messages
                if (msg_echo->flags & MSG_FLAG_BROADCAST) {
                    pthread_mutex_lock(&shared->clients->lock);
                    client_t *client = shared->clients->first;
                    while(client) {
                        message_t *msg_copy = message_copy(msg_echo);
                        message_queue_push_safe(&client->mq, msg_copy);
                        client = client->next;
                    }
                    pthread_mutex_unlock(&shared->clients->lock);
                    message_free(msg_echo);
                }
                else {
                    client_t *client = client_list_find_safe(
                        shared->clients,
                        msg_echo->client_id
                    );
                    message_queue_push_safe(&client->mq, msg_echo);
                }
            }
        }

        // run the game loop at shared->game_fps frame rate
        // send updates to clients
        // if (current_time >= next_frame_time) {
    }
}
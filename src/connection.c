#include "client.h"
#include "connection.h"
#include "message.h"
#include "shared.h"

#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

void *listener_thread(void *arg) {
    listener_shared_t *shared = arg;
    uint32_t current_client_id = 1;
    client_t *client;

    char buffer[shared->buffer_size];
    struct sockaddr_in cli_addr;
    
    printf("Listener thread running on port %d\n", shared->port);
    while (1) {
        int addr_size, msg_len;

        memset(&cli_addr, 0, sizeof(cli_addr));
        addr_size = sizeof(cli_addr);

        msg_len = recvfrom(
            shared->socket_fd,
            (char *)buffer,
            shared->buffer_size, 
            MSG_WAITALL,
            (struct sockaddr *) &cli_addr,
            &addr_size
        );
        if (msg_len == -1) {
            perror("Receive failed\n");
            exit(EXIT_FAILURE);
        }

        uint8_t type = buffer[0];
        int cid = *((uint32_t*)&buffer[1]);
        
        if (cid == 0) { // if client has no id
            if (client_list_full(shared->clients)) {
                printf("Client trying to join full server\n");
                msg_t *msg = (msg_t *)malloc(sizeof(msg_t));
                msg->type = MSG_SERVER_FULL;
                msg->flags = 0;
                msg->next = NULL;
                msg->data = NULL;

                // create dummy client for replying
                client_t *client = (client_t *)malloc(sizeof(client_t));
                client->id = 0;
                client->address = cli_addr.sin_addr.s_addr;
                client->port = cli_addr.sin_port;
                msg->client = client;

                msg_queue_push_last_safe(shared->mq_out, msg);
                continue;
            }

            cid = current_client_id;
            client = client_list_add_safe(
                shared->clients,
                current_client_id++,
                cli_addr.sin_addr.s_addr,
                cli_addr.sin_port
            );

            printf("New client %d\n", client->id);
        }
        else { // id client has id
            client = client_list_find_safe(shared->clients, cid);
            if (!client) {
                printf("Invalid client %d msg %d received\n", cid, type);
                continue;
            }
            client->last_receive_time = clock();
        }

        // construct message(s) and push to queue(s)
        if (type == MSG_CLIENT_INPUT) {
            msg_t *msg = (msg_t *)malloc(sizeof(msg_t));
            msg->type = MSG_CLIENT_INPUT;
            msg->flags = 0;
            msg->next = NULL;
            msg->client = client;

            uint8_t dgram_data = buffer[5];
            int8_t *data = (int8_t *)malloc(3);
            memset(data, 0, 3);
            if (dgram_data & MSG_INPUT_X_POS) data[0] += 1;
            if (dgram_data & MSG_INPUT_X_NEG) data[0] -= 1;
            if (dgram_data & MSG_INPUT_Y_POS) data[1] += 1;
            if (dgram_data & MSG_INPUT_Y_NEG) data[1] -= 1;
            if (dgram_data & MSG_INPUT_R_POS) data[2] += 1;
            if (dgram_data & MSG_INPUT_R_NEG) data[2] -= 1;
            msg->data = data;

            msg_queue_push_last_safe(shared->mq_in, msg);
        }
        else if (type == MSG_CLIENT_KEEPALIVE) {
            msg_t *msg = (msg_t *)malloc(sizeof(msg_t));
            msg->type = MSG_SERVER_ACK;
            msg->flags = 0;
            msg->next = NULL;
            msg->client = client;
            msg->data = NULL;

            msg_queue_push_last_safe(shared->mq_out, msg);
        }
        else if (type == MSG_CLIENT_CONNECT) {
            msg_t *msg = (msg_t *)malloc(sizeof(msg_t));
            msg->type = MSG_SERVER_CLIENT;
            msg->flags = 0;
            msg->next = NULL;
            msg->client = client;

            uint32_t *data = (uint32_t *)malloc(sizeof(uint32_t) * 2); // [cid, eid]
            data[0] = cid;
            msg->data = data;

            msg_queue_push_last_safe(shared->mq_in, msg);
        }
        else if (type == MSG_CLIENT_DISCONNECT) {
            msg_t *msg = (msg_t *)malloc(sizeof(msg_t));
            msg->type = MSG_SERVER_REMOVE;
            msg->flags = 0;
            msg->next = NULL;
            msg->client = NULL;
            msg->data = client->entity;
            msg_queue_push_last_safe(shared->mq_in, msg);

            // remove client
            printf("Disconnect client %d\n", client->id);
            client_list_remove_safe(shared->clients, client->id);
        }
        else {
            printf("Unknown msg %d from client %d\n", type, cid);
        }
    }
}

void *sender_thread(void *arg) {
    sender_shared_t *shared = arg;
    msg_queue_t mq_sender = {0, NULL, NULL, NULL};
    // msg_queue_t mq_sender_pnd = {0, NULL, NULL, NULL}; // pending reliable messages

    struct sockaddr_in addr_cli;
    char buffer[shared->buffer_size];
    int buffer_size;

    printf("Sender thread running\n");
    while(1) {
        msg_queue_fetch_safe(&mq_sender, shared->mq_out);
        if (mq_sender.size == 0) {
            continue;
        }

        msg_t *msg, *next;
        do {
            msg = msg_queue_pop_first(&mq_sender);
            // if (!msg->client) {
            //     printf("Client removed before send msg %d\n", msg->type);
            //     next = msg->next;
            //     msg_free(msg);
            //     msg = next;
            // }

            // put message to buffer
            buffer_size = 0;
            if (msg->type == MSG_SERVER_MOVE) {
                char *data = (char *)msg->data;
                buffer_size = 21; // 1 + 4 + 4 + 4 + 4 + 4
                buffer[0] = msg->type; // msg type
                memcpy(&buffer[1], &data[0], 4); // eid
                memcpy(&buffer[5], &data[4], 4); // pos x
                memcpy(&buffer[9], &data[8], 4); // pos y
                memcpy(&buffer[13], &data[12], 4); // vel x
                memcpy(&buffer[17], &data[16], 4); // vel y
            }
            else if (msg->type == MSG_SERVER_ADD) {
                char *data = (char *)msg->data;
                buffer_size = 14; // 1 + 1 + 4 + 4 + 4
                buffer[0] = msg->type; // msg type
                buffer[1] = data[0]; // entity type
                memcpy(&buffer[2], &data[1], 4); // eid
                memcpy(&buffer[6], &data[5], 4); // pos x
                memcpy(&buffer[11], &data[9], 4); // pos y
            }
            else if (msg->type == MSG_SERVER_REMOVE) {
                uint32_t *data = (uint32_t *)msg->data;
                buffer_size = 5; // 1 + 4
                buffer[0] = msg->type; // msg type
                memcpy(&buffer[1], &msg->data, 4); // eid
            }
            else if (msg->type == MSG_SERVER_ACK) {
                buffer_size = 1;
                buffer[0] = msg->type; // msg type
            }
            else if(msg->type == MSG_SERVER_FULL) {
                buffer[0] = msg->type;
                buffer_size += sizeof(msg->type);
                free(msg->client); // free dummy client
            }
            else if (msg->type == MSG_SERVER_CLIENT) {
                uint32_t *data = (uint32_t *)msg->data;
                buffer_size = 9;
                buffer[0] = msg->type; // msg type
                memcpy(&buffer[1], &data[0], 4); // cid
                memcpy(&buffer[5], &data[1], 4); // eid
            }
            else {
                printf("Trying to send unimplemented msg %d\n", msg->type);
                exit(EXIT_FAILURE);
            }

            if (!msg->flags) {
                memset(&addr_cli, 0, sizeof(addr_cli));
                addr_cli.sin_family = AF_INET;
                addr_cli.sin_addr.s_addr = msg->client->address;
                addr_cli.sin_port = msg->client->port;

                sendto(
                    shared->socket_fd,
                    &buffer,
                    buffer_size,
                    MSG_CONFIRM,
                    (const struct sockaddr *)&addr_cli,
                    sizeof(addr_cli)
                );
                // printf("sent msg %d\n", msg->type);
            }
            else if (msg->flags | MSG_FLAG_BROADCAST) {
                pthread_mutex_lock(shared->clients->lock);

                client_t *client = shared->clients->first;
                struct sockaddr_in addr_bc;
                while (client) {
                    memset(&addr_bc, 0, sizeof(addr_bc));
                    addr_bc.sin_family = AF_INET;
                    addr_bc.sin_addr.s_addr = client->address;
                    addr_bc.sin_port = client->port;

                    sendto(
                        shared->socket_fd,
                        &buffer,
                        buffer_size,
                        MSG_CONFIRM,
                        (const struct sockaddr *)&addr_bc,
                        sizeof(addr_bc)
                    );

                    client = client->next;
                }

                pthread_mutex_unlock(shared->clients->lock);
            }

            next = msg->next;
            msg_free(msg);
            msg = next;
        } while (msg);
    }
}

void *timeout_thread(void *arg) {
    timeout_shared_t *shared = (timeout_shared_t *)arg;

    printf("Timeout cleaner thread running\n");
    while (1) {
        sleep(shared->timeout_sec);

        pthread_mutex_lock(shared->clients->lock);

        time_t cur_time = clock();
        client_t *client = shared->clients->first;
        while (client) {
            // check is client has reached timeout
            float last_received = (float)((cur_time - client->last_receive_time) / CLOCKS_PER_SEC);
            if (last_received >= shared->timeout_sec) {
                // send message to game thread to remove player entity
                msg_t *msg = (msg_t *)malloc(sizeof(msg_t));
                msg->type = MSG_SERVER_REMOVE;
                msg->flags = 0;
                msg->next = NULL;
                msg->client = NULL;
                msg->data = client->entity;
                msg_queue_push_last_safe(shared->mq_in, msg);

                // remove client
                printf("Timeout client %d\n", client->id);
                client_t *next = client->next;
                client_list_remove(shared->clients, client->id);
                // continue looping
                client = next;
            }
            else {
                client = client->next;
            }
        }

        pthread_mutex_unlock(shared->clients->lock);
    }
}

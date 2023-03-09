#include "client.h"
#include "common.h"
#include "connection.h"
#include "shared.h"

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

void *listener_thread(void *arg) {
    listener_shared_t *shared = arg;
    uint32_t current_client_id = 1;

    int client_socket_fd;
    struct sockaddr_in client_addr;
    socklen_t client_socklen = (socklen_t)sizeof(client_addr);
    
    int error = listen(shared->socket_fd, shared->max_waiting_connections);
    if (error == -1) {
        fprintf(stderr, "Listener listen failed\n");
        exit(EXIT_FAILURE);
    }
    
    printf("Listener thread running on port %d\n", shared->port);
    while (1) {
        // wait for connections and accept
        client_socket_fd = accept(
            shared->socket_fd,
            (struct sockaddr *)&client_addr,
            &client_socklen
        );
        if (client_socket_fd < 0) {
            fprintf(stderr, "Listener accept failed\n");
            exit(EXIT_FAILURE);
        }

        // check if max clients
        if (is_client_list_full_safe(shared->clients)) {
            // send back MSG_SERVER_FULL
            printf("Client trying to join full server\n");
            char buffer[1];
            buffer[0] = MSG_SERVER_FULL;
            send(
                client_socket_fd,
                &buffer,
                1,
                MSG_CONFIRM | MSG_DONTWAIT
            );

            close(client_socket_fd);
            continue;
        }

        // make connection non blocking
        int flags = fcntl(client_socket_fd, F_GETFL);
        if (flags == -1) {
            fprintf(stderr, "Listener getfl failed\n");
            exit(EXIT_FAILURE);
        }
        fcntl(client_socket_fd, F_SETFL, flags | O_NONBLOCK);

        // add new client to list
        client_t *client = (client_t *)malloc(sizeof(client_t));
        check_malloc(client);
        *client = (client_t) {
            .mq = {0},
            .socket_fd = client_socket_fd,
            .id = current_client_id++,
            .close = false
        };
        if (pthread_mutex_init(&client->mq.lock, NULL) != 0) {
            fprintf(stderr, "Client mutex init failed\n");
            exit(EXIT_FAILURE);
        }
        client_list_add_safe(shared->clients, client);

        // start client thread with shared data
        client_shared_t *client_shared = (client_shared_t *)malloc(
            sizeof(client_shared_t)
        );
        check_malloc(client_shared);
        *client_shared = (client_shared_t) {
            .in_mq = shared->in_mq,
            .out_mq = shared->out_mq,
            .clients = shared->clients,
            .client = client,
            .buffer_size = shared->buffer_size
        };
        pthread_create(
            &client->thread_id,
            NULL,
            client_thread,
            client_shared
        );
    }
}

void *client_thread(void *arg) {
    client_shared_t *shared = (client_shared_t *)arg;
    client_t *client = shared->client;
    message_queue_t local_mq = {0};

    char connected = 1;
    char buffer[shared->buffer_size];
    int buffer_size;

    printf("Client %d thread started\n", client->id);
    while (connected) {
        // first receive messages from socket
        buffer_size = read(client->socket_fd, buffer, shared->buffer_size);
        if (buffer_size == -1) {
            if (errno != EWOULDBLOCK) {
                fprintf(stderr, "Client %d read failed\n", client->id);
                exit(EXIT_FAILURE);
            }
        }
        else if (buffer_size > 0) {
            uint8_t type = buffer[0];
            printf("Client received msg type %d\n", type);

            message_t *msg = (message_t *)malloc(sizeof(message_t));
            *msg = (message_t) {
                .next = NULL,
                .client_id = client->id,
                .type = type,
                .data = NULL,
                .flags = 0
            };

            message_queue_push_safe(shared->in_mq, msg);
        }
        else {
            // socket read EOF, client closes connection
            break;
        }
        
        // then check if there are messages to send
        message_queue_fetch_safe(&local_mq, &client->mq);
        if (!local_mq.size) {
            continue;
        }

        message_t *msg = message_queue_pop(&local_mq);
        while (msg) {
            buffer_size = 0;

            buffer[0] = msg->type;
            buffer_size += sizeof(msg->type);

            send(
                client->socket_fd,
                &buffer,
                buffer_size,
                MSG_CONFIRM | MSG_DONTWAIT
            );

            message_free(msg);
            msg = message_queue_pop(&local_mq);
        }

        // close client if close flag is set
        if (client->close) {
            break;
        }
    }

    // cleanup before closing thread
    printf("Closing client %d thread\n", client->id);
    message_queue_clear(&client->mq);
    pthread_mutex_destroy(&client->mq.lock);
    client_list_remove_safe(shared->clients, client->id);
    close(client->socket_fd);
    free(shared);
}
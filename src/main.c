#include "client.h"
#include "connection.h"
#include "game.h"
#include "message.h"
#include "shared.h"

#include <arpa/inet.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

const int DEFAULT_PORT = 8080;
const int GAME_FPS = 10;
const int MAX_CLIENTS = 4;
const int MAX_WAITING_CONNECTIONS = 4;
const char *PORT_ENV_NAME = "PORT";
const int TCP_BUFFER_SIZE = 256;

int get_port_env();

int main(int argc, char *argv[]) {
    uint16_t port;
    int socket_fd, error;
    struct sockaddr_in server_addr;
    socklen_t socklen;
    pthread_t game_id, listener_id;

    message_queue_t in_mq = {0};
    message_queue_t out_mq = {0};
    client_list_t clients = {
        .max_size = MAX_CLIENTS
    };

    port = get_port_env();
    if (port > 0) {
        port = DEFAULT_PORT;
    }

    error = pthread_mutex_init(&in_mq.lock, NULL);
    error |= pthread_mutex_init(&out_mq.lock, NULL);
    error |= pthread_mutex_init(&clients.lock, NULL);
    if (error != 0) {
        fprintf(stderr, "Mutex init failed\n");
        exit(EXIT_FAILURE);
    }

    // create and bind server socket
    socket_fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (socket_fd == -1) {
        fprintf(stderr, "Socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    socklen = sizeof(struct sockaddr_in);
    memset(&server_addr, 0, socklen);
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
    server_addr.sin_port = htons(port);

    if (bind(socket_fd, (struct sockaddr *)&server_addr, socklen) == -1) {
        fprintf(stderr, "Socket bind failed\n");
        exit(EXIT_FAILURE);
    }

    // start game thread with shared data
    game_shared_t gs = {
        .in_mq = &in_mq,
        .out_mq = &out_mq,
        .clients = &clients,
        .game_fps = GAME_FPS
    };
    error = pthread_create(&game_id, NULL, game_thread, &gs);
    if (error) {
        fprintf(stderr, "Game thread create failed with code %d\n", error);
        exit(EXIT_FAILURE);
    }

    // start connection listener thread with shared data
    listener_shared_t ls = {
        .in_mq = &in_mq,
        .out_mq = &out_mq,
        .clients = &clients,
        .socket_fd = socket_fd,
        .buffer_size = TCP_BUFFER_SIZE,
        .port = port,
        .max_clients = MAX_CLIENTS,
        .max_waiting_connections = MAX_WAITING_CONNECTIONS
    };
    error = pthread_create(&listener_id, NULL, listener_thread, &ls);
    if (error) {
        fprintf(stderr, "Listener thread create failed with code %d\n", error);
        exit(EXIT_FAILURE);
    }

    // wait until game thread ends
    pthread_join(game_id, NULL);

    printf("Closing server\n");
    close(socket_fd);

    return 0;
}

int get_port_env() {
    char *env = getenv(PORT_ENV_NAME);
    if (env) {
        char *ptr;
        return strtol(env, &ptr, 10);
    }
    return -1;
}
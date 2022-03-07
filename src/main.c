#include "client.h"
#include "connection.h"
#include "game.h"
#include "message.h"
#include "shared.h"

#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define PORT 6666
#define MAX_CONNECTIONS 3
#define UDP_BUFFER_SIZE 512
#define CLIENT_TIMEOUT_SEC 3
#define GAME_FPS 10

int main(int argc, char *argv[]) {
    int socket_fd, error;
    struct sockaddr_in serv_addr;

    pthread_mutex_t in_mutex, out_mutex, cli_mutex;
    pthread_t game_id, listener_id, sender_id, timeout_id;

    msg_queue_t mq_in = {0, NULL, NULL, &in_mutex};
    msg_queue_t mq_out = {0, NULL, NULL, &out_mutex};
    client_list_t clients = {0, MAX_CONNECTIONS, NULL, NULL, &cli_mutex};

    error = pthread_mutex_init(&in_mutex, NULL);
    error |= pthread_mutex_init(&out_mutex, NULL);
    error |= pthread_mutex_init(&cli_mutex, NULL);
    if (error != 0) {
        perror("Mutex init failed\n");
        exit(EXIT_FAILURE);
    }

    socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd == -1) {
        perror("Socket creation failed\n");
        exit(EXIT_FAILURE);
    }

    memset(&serv_addr, 0, sizeof(struct sockaddr_in));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    error = bind(
        socket_fd,
        (const struct sockaddr *)&serv_addr, 
        sizeof(serv_addr)
    );
    if (error == -1) {
        perror("Socket bind failed\n");
        exit(EXIT_FAILURE);
    }

    // start game thread
    game_shared_t gs = {
        &mq_in,
        &mq_out,
        GAME_FPS
    };
    pthread_create(&game_id, NULL, game_thread, &gs);

    // start udp sender thread
    sender_shared_t ss = {
        &mq_out,
        &clients,
        socket_fd,
        UDP_BUFFER_SIZE
    };
    pthread_create(&sender_id, NULL, sender_thread, &ss);

    // start udp listener thread
    listener_shared_t ls = {
        &mq_in,
        &mq_out,
        &clients,
        socket_fd,
        UDP_BUFFER_SIZE,
        PORT,
        MAX_CONNECTIONS
    };
    pthread_create(&listener_id, NULL, listener_thread, &ls);

    // start connection timeout cleaner thread
    timeout_shared_t ts = {
        &mq_in,
        &clients,
        CLIENT_TIMEOUT_SEC
    };
    pthread_create(&timeout_id, NULL, timeout_thread, &ts);

    printf("Press ENTER key to exit program\n");  
    getchar();

    close(socket_fd);
    return 0;
}
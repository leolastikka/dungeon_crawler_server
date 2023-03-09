#include <stdint.h>
#include <sys/socket.h>

#ifndef CONNECTION_H
#define CONNECTION_H

void *client_thread(void *arg);
void *listener_thread(void *arg);
void *sender_thread(void *arg);

#endif
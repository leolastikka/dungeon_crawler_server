#include <stdint.h>
#include <sys/socket.h>

#ifndef CONNECTION_H
#define CONNECTION_H

void *listener_thread(void *arg);
void *sender_thread(void *arg);
void *timeout_thread(void *arg);

#endif
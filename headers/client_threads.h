#ifndef CLIENT_THREADS_H
#define CLIENT_THREADS_H
#include "client_commands.h"
#include "messages_queue.h"
#include <sys/inotify.h>

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))
extern int is_sync_running;
extern int is_inotify_running;
extern int is_messages_running;
extern int is_rcv_propagation_running;
void *sync_dir_thread(void *arg);
void *inotify_thread(void *arg);
void *messages_thread(void *arg);
void *rcv_propagation(void *arg);
void monitor_sync_dir(int sockfd);

#endif

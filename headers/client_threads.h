#ifndef CLIENT_THREADS_H
#define CLIENT_THREADS_H
#include "client_commands.h"
#include "messages_queue.h"
#include <pthread.h>
#include <string.h>
#include <sys/inotify.h>
#include <time.h>

#define HEARTBEAT_PORT 50001
#define HEARTBEAT_INTERVAL 3
#define HEARTBEAT_TIMEOUT 5

#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))
extern int is_sync_running;
extern int is_inotify_running;
extern int is_messages_running;
extern int is_rcv_propagation_running;

void *sync_dir_thread(void *arg);
void *inotify_thread(void *arg);
void *messages_thread(void *arg);
void *rcv_propagation_thread(void *arg);
void *heartbeat_thread(void *arg);
void monitor_sync_dir();
int client_connect(const char *server_ip, int port);

#endif

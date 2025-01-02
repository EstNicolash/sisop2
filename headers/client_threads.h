#ifndef CLIENT_THREADS_H
#define CLIENT_THREADS_H
#include "client_commands.h"
#include "messages_queue.h"
#include <sys/inotify.h>

#include <pthread.h>
#include <string.h>
#include <time.h>
#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))
extern int is_sync_running;
extern int is_inotify_running;
extern int is_messages_running;
extern int is_rcv_propagation_running;

extern int sockfd, prop_read_sockfd, prop_write_sockfd;
extern int port;
extern char server_ip[256];
extern char next_server_ip[256];

void *sync_dir_thread(void *arg);
void *inotify_thread(void *arg);
void *messages_thread(void *arg);
void *rcv_propagation_thread(void *arg);
void monitor_sync_dir();
int client_connect(const char *server_ip, int port);

#endif

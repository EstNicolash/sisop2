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

typedef struct {
  char file[MAX_PAYLOAD_SIZE * 2];
  time_t timestamp;
} IgnoreList;

extern IgnoreList ignore_files[50];
extern pthread_mutex_t ignore_mutex;
#define IGNORE_TIME 2 // seconds to ignore after upload
void *sync_dir_thread(void *arg);
void *inotify_thread(void *arg);
void *messages_thread(void *arg);
void *rcv_propagation_thread(void *arg);
void monitor_sync_dir(int sockfd);

void add_to_ignore_list(const char *file);
int is_ignored(const char *file);

#endif

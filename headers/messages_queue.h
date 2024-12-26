#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H
#import "protocol.h"
#import "pthread.h"

extern pthread_mutex_t queue_mutex;

struct message_queue {
  int msg_type;
  struct message_queue *next;
};

struct message_queue *msg_queue_insert(int type);
struct message_queue *msg_queue_remove();
int *msg_queue_is_empty();
int is_valid_type(int type);

#endif

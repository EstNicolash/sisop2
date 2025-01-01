#ifndef MESSAGE_QUEUE_H
#define MESSAGE_QUEUE_H
#include "protocol.h"
#include "pthread.h"

extern pthread_mutex_t queue_mutex;
extern struct message_queue *head;

struct message_queue {
  int msg_type;
  char msg_info[MAX_PAYLOAD_SIZE];
  struct message_queue *next;
};

int msg_queue_insert(int type, char msg_info[MAX_PAYLOAD_SIZE]);
int msg_queue_insert_start(int type, char msg_info[MAX_PAYLOAD_SIZE]); //??????/
struct message_queue *msg_queue_remove();
int is_empty_msg_queue();
int is_valid_type(int type);

#endif

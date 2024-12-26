#include "../headers/messages_queue.h"
#include <stdio.h>
#include <stdlib.h>

pthread_mutex_t queue_mutex = PTHREAD_MUTEX_INITIALIZER;
static struct message_queue *head = NULL;

static struct message_queue *create_node(int type) {
  struct message_queue *new_node =
      (struct message_queue *)malloc(sizeof(struct message_queue));
  if (!new_node) {
    perror("Failed to allocate memory for new node");
    return NULL;
  }
  new_node->msg_type = type;
  new_node->next = NULL;
  return new_node;
}

struct message_queue *msg_queue_insert(int type) {
  if (!is_valid_type(type)) {
    fprintf(stderr, "Invalid message type: %d\n", type);
    return NULL;
  }

  struct message_queue *new_node = create_node(type);
  if (!new_node)
    return NULL;

  pthread_mutex_lock(&queue_mutex);

  if (!head) {
    head = new_node;
  } else {
    struct message_queue *temp = head;
    while (temp->next) {
      temp = temp->next;
    }
    temp->next = new_node;
  }

  pthread_mutex_unlock(&queue_mutex);

  return new_node;
}

struct message_queue *msg_queue_remove() {
  pthread_mutex_lock(&queue_mutex);

  if (!head) {
    pthread_mutex_unlock(&queue_mutex);
    return NULL;
  }

  struct message_queue *temp = head;
  head = head->next;

  pthread_mutex_unlock(&queue_mutex);
  return temp;
}

int is_empty_msg_queue() {
  int is_empty = 0;
  pthread_mutex_lock(&queue_mutex);
  if (head == NULL)
    is_empty = -1;
  pthread_mutex_unlock(&queue_mutex);
  return is_empty;
}

int is_valid_type(int type) {
  switch (type) {
  case METADATA:
  case END_OF_FILE:
  case C_SEND_ID:
  case C_UPLOAD:
  case C_DOWNLOAD:
  case C_DELETE:
  case C_LIST_SERVER:
  case C_EXIT:
  case C_GET_SYNC_DIR:
  case ANYTHING:
    return 0;
  default:
    return -1;
  }
}

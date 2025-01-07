#include "../headers/connection_map.h"

ConnectionMap connection_map;
pthread_mutex_t connection_mutex = PTHREAD_MUTEX_INITIALIZER;

void connection_map_init() {
  for (int i = 0; i < MAX_CLIENTS; i++) {
    connection_map.map[i] = NULL;
  }
}

int hash_function(const char user_id[MAX_FILENAME_SIZE]) {
  unsigned long hash = 5381;
  for (int i = 0; i < MAX_FILENAME_SIZE && user_id[i] != '\0'; i++) {
    hash = ((hash << 5) + hash) + user_id[i];
  }
  return hash % MAX_CLIENTS;
}

void connection_map_insert(const char user_id[MAX_FILENAME_SIZE],
                           const int normal_sockfd,
                           const int propagation_read_sockfd,
                           const int propagation_write_sockfd) {
  pthread_mutex_lock(&connection_mutex);

  int index = hash_function(user_id);
  ConnectionInfo info = {0};
  // strncpy(info.user_id, user_id, MAX_FILENAME_SIZE);

  snprintf(info.user_id, MAX_FILENAME_SIZE, "%s", user_id);
  fprintf(stderr, "Connection map: %d,%d,%d\n", normal_sockfd,
          propagation_read_sockfd, propagation_write_sockfd);
  info.normal_sockfd = normal_sockfd;
  info.propagation_read_sockfd = propagation_read_sockfd;
  info.propagation_write_sockfd = propagation_write_sockfd;

  connection_map.map[index] =
      connection_list_insert(connection_map.map[index], info);

  pthread_mutex_unlock(&connection_mutex);
}

void connection_map_delete(const char user_id[MAX_FILENAME_SIZE],
                           const int normal_sockfd) {
  pthread_mutex_lock(&connection_mutex);

  int index = hash_function(user_id);

  if (connection_map.map[index] != NULL) {
    connection_map.map[index] =
        connection_list_delete(connection_map.map[index], normal_sockfd);
  }

  pthread_mutex_unlock(&connection_mutex);
}

int connection_map_count_user_connections(
    const char user_id[MAX_FILENAME_SIZE]) {
  pthread_mutex_lock(&connection_mutex);

  int count = 0;
  int index = hash_function(user_id);
  if (connection_map.map[index] != NULL) {
    count = connection_list_count_user_connections(connection_map.map[index],
                                                   user_id);
  }

  pthread_mutex_unlock(&connection_mutex);
  return count;
}

ConnectionInfo *
connection_map_search_other(const char user_id[MAX_FILENAME_SIZE],
                            const int normal_sockfd) {
  pthread_mutex_lock(&connection_mutex);

  int index = hash_function(user_id);
  ConnectionInfo *other_connection = NULL;

  if (connection_map.map[index] != NULL) {
    // Search using connection_list_search_other
    other_connection = connection_list_search_other(connection_map.map[index],
                                                    user_id, normal_sockfd);
  }

  pthread_mutex_unlock(&connection_mutex);
  return other_connection;
}

ConnectionList *connection_list_insert(ConnectionList *list,
                                       ConnectionInfo info) {
  ConnectionList *new_node = (ConnectionList *)malloc(sizeof(ConnectionList));
  if (new_node == NULL) {
    perror("Failed to insert connection");
    return list;
  }
  new_node->connection = (ConnectionInfo *)malloc(sizeof(ConnectionInfo));
  if (new_node->connection == NULL) {
    perror("Failed to allocate connection info");
    free(new_node);
    return list;
  }
  *new_node->connection = info;
  new_node->next = list;
  return new_node;
}
ConnectionList *connection_list_delete(ConnectionList *list,
                                       const int normal_sockfd) {
  ConnectionList *current = list;
  ConnectionList *prev = NULL;
  while (current != NULL) {
    if (current->connection->normal_sockfd == normal_sockfd) {
      if (prev == NULL) {
        ConnectionList *temp = current;
        list = current->next;
        free(temp->connection);
        free(temp);
      } else {
        prev->next = current->next;
        free(current->connection);
        free(current);
      }
      return list;
    }
    prev = current;
    current = current->next;
  }
  return list;
}

int connection_list_count_user_connections(
    ConnectionList *list, const char user_id[MAX_FILENAME_SIZE]) {
  int count = 0;
  ConnectionList *current = list;
  while (current != NULL) {
    if (strcmp(current->connection->user_id, user_id) == 0) {
      count++;
    }
    current = current->next;
  }
  return count;
}

ConnectionInfo *
connection_list_search_other(ConnectionList *list,
                             const char user_id[MAX_FILENAME_SIZE],
                             const int normal_sockfd) {
  ConnectionList *current = list;
  while (current != NULL) {
    if (strcmp(current->connection->user_id, user_id) == 0 &&
        current->connection->normal_sockfd != normal_sockfd) {
      return current->connection; // Return the other connection info
    }
    current = current->next;
  }
  return NULL; // No other connection found
}

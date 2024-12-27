#ifndef CONNECTION_MAP_H
#define CONNECTION_MAP_H

#include "file_manager.h"
#include "protocol.h"
#include "pthread.h"

#define MAX_CONNECTIONS_PER_CLIENT 2
#define MAX_CLIENTS 1000

typedef struct {
  char user_id[MAX_FILENAME_SIZE];
  int normal_sockfd;
  int propagation_read_sockfd;
  int propagation_write_sockfd;
} ConnectionInfo;

typedef struct connection_list {
  ConnectionInfo *connection;
  struct connection_list *next;
} ConnectionList;

typedef struct {
  ConnectionList *map[MAX_CLIENTS];
} ConnectionMap;

extern ConnectionMap connection_map;
extern pthread_mutex_t connection_mutex;

// Connection List functions
ConnectionList *
connection_list_insert(ConnectionList *list,
                       ConnectionInfo info); // Insert in start of the list
ConnectionList *
connection_list_delete(ConnectionList *list,
                       int normal_sockfd); // Delete by normal_sockfd
int connection_list_count_user_connections(
    ConnectionList *list, const char user_id[MAX_FILENAME_SIZE]);
ConnectionInfo *connection_list_search_other(
    ConnectionList *list, const char user_id[MAX_FILENAME_SIZE],
    const int normal_sockfd); // return the info of the other connectin of the
                              // user, if exists

// Connection Map functions:
void connection_map_init();

void connection_map_delete(const char user_id[MAX_FILENAME_SIZE],
                           const int normal_sockfd);

void connection_map_insert(const char user_id[MAX_FILENAME_SIZE],
                           const int normal_sockfd,
                           const int propagation_read_sockfd,
                           const int propagation_write_sockfd);

int connection_map_count_user_connections(
    const char user_id[MAX_FILENAME_SIZE]);

ConnectionInfo *connection_map_search_other(
    const char user_id[MAX_FILENAME_SIZE],
    const int normal_sockfd); // return the info of the other connectin of the
                              // user, if exists
int hash_function(const char user_id[MAX_FILENAME_SIZE]);
#endif

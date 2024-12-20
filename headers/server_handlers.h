#ifndef SERVER_HANDLER_H
#define SERVER_HANDLER_H
#include "protocol.h"
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define MAX_CONNECTIONS_PER_CLIENT 2
#define MAX_CLIENTS 1000

typedef struct {
  char user_id[MAX_PAYLOAD_SIZE];
  int connection_count;
} Client;

extern Client manage_clients_connections[MAX_CLIENTS];
extern int client_count;
extern pthread_mutex_t manage_clients_mutex;

int server_handles_download(int sockfd, const char user_id[MAX_FILENAME_SIZE]);
int server_handles_upload(int sockfd, const char user_id[MAX_FILENAME_SIZE]);
int server_handles_list_server(int sockfd,
                               const char user_id[MAX_FILENAME_SIZE]);
int server_handles_id(int sockfd, char user_id[MAX_FILENAME_SIZE]);
int server_handles_delete(int sockfd, const char user_id[MAX_FILENAME_SIZE]);
int server_handles_get_sync_dir(int sockfd);
int update_connection_count(const char *user_id, int client_num);

#endif

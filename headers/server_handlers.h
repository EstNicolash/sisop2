#ifndef SERVER_HANDLER_H
#define SERVER_HANDLER_H
#include "connection_map.h"
#include "file_manager.h"
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

// Propagate received file to another client connection
int propagate_to_client(int sockfd, const char user_id[MAX_FILENAME_SIZE],
                        const char filename[MAX_FILENAME_SIZE]);

// Propagate delete file to another client connection
int propagate_delete(int sockfd, const char user_id[MAX_FILENAME_SIZE],
                     const char filename[MAX_FILENAME_SIZE]);

int server_handles_download(int sockfd, const char user_id[MAX_FILENAME_SIZE]);

int server_handles_upload(int sockfd, char user_id[MAX_FILENAME_SIZE]);

int server_handles_list_server(int sockfd,
                               const char user_id[MAX_FILENAME_SIZE]);
int server_handles_id(int sockfd, char user_id[MAX_FILENAME_SIZE]);
int server_handles_delete(int sockfd, const char user_id[MAX_FILENAME_SIZE]);
int server_handles_get_sync_dir(int sockfd);

// Remove user connection
int remove_connection(const char user_id[MAX_FILENAME_SIZE], int normal_sockfd);

// Add user connection
int add_connection(const char user_id[MAX_FILENAME_SIZE], int normal_sockfd,
                   int propagation_r_sockfd, int propagation_w_sockfd);
void map_init();

int send_replica(char dir[MAX_FILENAME_SIZE]);

#endif

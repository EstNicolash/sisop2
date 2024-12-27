#ifndef SERVER_HANDLER_H
#define SERVER_HANDLER_H
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

int server_handles_download(int sockfd, const char user_id[MAX_FILENAME_SIZE]);
int server_handles_upload(int sockfd, const char user_id[MAX_FILENAME_SIZE]);
int server_handles_list_server(int sockfd,
                               const char user_id[MAX_FILENAME_SIZE]);
int server_handles_id(int sockfd, char user_id[MAX_FILENAME_SIZE]);
int server_handles_delete(int sockfd, const char user_id[MAX_FILENAME_SIZE]);
int server_handles_get_sync_dir(int sockfd);
int remove_connection(const char user_id[MAX_FILENAME_SIZE], int normal_sockfd);
int add_connection(const char user_id[MAX_FILENAME_SIZE], int normal_sockfd,
                   int propagation_sockfd);

#endif

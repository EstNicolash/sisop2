#ifndef CLIENT_MESSAGES_H
#define CLIENT_MESSAGES_H
#include "file_manager.h"
#include "protocol.h"
int client_send_id(int sockfd, char client_id[MAX_FILENAME_SIZE]);
int client_upload_file(int sockfd, char filename[MAX_FILENAME_SIZE]);
int client_download_file(int sockfd, char filename[MAX_FILENAME_SIZE]);
int client_delete_file(int sockfd, char filename[MAX_FILENAME_SIZE]);
int client_list_server(int sockfd);
int client_list_client(int sockfd);
#endif

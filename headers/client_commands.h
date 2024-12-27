#ifndef CLIENT_MESSAGES_H
#define CLIENT_MESSAGES_H
#include "file_manager.h"
#include "protocol.h"

#define SYNC_DIR "sync_dir"

int client_send_id(int sockfd, char client_id[MAX_FILENAME_SIZE]);
int client_upload_file(int sockfd, char filename[MAX_FILENAME_SIZE]);
int client_download_file(int sockfd, char filename[MAX_FILENAME_SIZE],
                         char *dir);
int client_delete_file(int sockfd, char filename[MAX_FILENAME_SIZE]);
int client_list_server(int sockfd);
int client_exit(int sockfd);
int client_list_client();
int get_sync_dir(int sockfd);
int rcv_propagation(int sockfd);
#endif

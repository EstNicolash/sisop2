#ifndef CLIENT_MESSAGES_H
#define CLIENT_MESSAGES_H
#include "file_manager.h"
#include "protocol.h"

typedef struct {
  char file[MAX_PAYLOAD_SIZE * 2];
  time_t timestamp;
} IgnoreList;

extern IgnoreList ignore_files[50];
extern pthread_mutex_t ignore_mutex;

#define IGNORE_TIME 2 // seconds to ignore after upload
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
int client_rcv_propagation(int sockfd);
int client_delete_propagation(int sockfd);

void add_to_ignore_list(const char *file);
int is_ignored(const char *file);
#endif

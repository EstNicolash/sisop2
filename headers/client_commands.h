#ifndef CLIENT_MESSAGES_H
#define CLIENT_MESSAGES_H
#include "file_manager.h"
#include "protocol.h"

#define IGNORE_TIME 2 // seconds to ignore after upload
#define MAX_IGNORE_FILES 50

typedef struct {
  char file[256];
  time_t timestamp;
} TimedIgnoreEntry;

typedef struct {
  char file[256];
} IgnoreEntry;

extern int sockfd, prop_read_sockfd, prop_write_sockfd;
extern int port;
extern char server_ip[256];
extern char next_server_ip[256];
extern IgnoreEntry ignore_files[MAX_IGNORE_FILES];
extern TimedIgnoreEntry timed_ignore_files[MAX_IGNORE_FILES];
extern pthread_mutex_t ignore_mutex;
extern char client_id[1024];

int client_init_msg();
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

void add_to_timed_ignore_list(const char *file);
void add_to_ignore_list(const char *file);
void remove_from_ignore_list(const char *file);
int is_timed_ignored(const char *file);
int is_ignored(const char *file);
#endif

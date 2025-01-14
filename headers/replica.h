#ifndef REPLICA_H
#define REPLICA_H
#include "election.h"
#include "file_manager.h"
#define REPLICA_PORT 51000
extern int replica_sockets[MAX_SERVERS];
void *replica_handler(void *arg);
int setup_replica_socket();
void *listen_for_replica_connection(void *arg);
int propagate_to_backup(int sockfd, const char user_id[MAX_FILENAME_SIZE],
                        const char filename[MAX_FILENAME_SIZE]);
int replica_connect(const char *server_ip);
int propagate_delete_to_backup(int sockfd,
                               const char user_id[MAX_FILENAME_SIZE],
                               const char filename[MAX_FILENAME_SIZE]);
#endif

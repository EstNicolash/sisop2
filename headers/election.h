#ifndef ELECTION_H
#define ELECTION_H
#include <pthread.h>
#define CONFIG_FILE_NAME "server_config.csv"
#define ELECTION_PORT2 50000
#define HEARTBEAT_PORT2 50001
#define REPLICA_PORT2 50010
#define REPLICA_SEND_PORT2 50011

#define ELECTION_PORT 50020
#define HEARTBEAT_PORT 50021
#define REPLICA_PORT 50011
#define REPLICA_SEND_PORT 50010

#define MAX_LINE_LENGTH 256
#define MAX_SERVERS 3

#define HEARTBEAT_INTERVAL 3 // Send heartbeat every 3 seconds
#define HEARTBEAT_TIMEOUT 5  // Timeout after 5 seconds

extern int read_listen_sockfd;
extern int heartbeat_sockfd;
extern int elected_server;
extern int next_server;
extern int is_participating;
extern int server_id;
extern char server_ips[MAX_SERVERS][MAX_LINE_LENGTH];
extern int alive_servers[MAX_SERVERS];
extern int total_servers;

extern pthread_mutex_t election_mutex;
extern pthread_cond_t election_cond;

struct election_msg {
  int elected;
  int id;
};

void read_server_config();
void setup_election_socket(int port);
void start_election();
void send_election_message(struct election_msg election_msg);
struct election_msg receive_election_message();
void *handle_election();
//
void get_local_ip(char *buffer);
void set_servers();
//
void *send_heartbeat(void *arg);
void *listen_for_heartbeat(void *arg);
void *replica_handler(void *arg);

#endif

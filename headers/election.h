#ifndef ELECTION_H
#define ELECTION_H

#define CONFIG_FILE_NAME "server_config.csv"
#define ELECTION_PORT 50000
#define HEARTBEAT_PORT 50001

#define MAX_LINE_LENGTH 256
#define MAX_SERVERS 3

extern int read_listen_sockfd;
extern int heartbeat_sockfd;
extern int elected_server;
extern int next_server;
extern int is_participating;
extern int server_id;
extern char sever_ips[MAX_SERVERS][MAX_LINE_LENGTH];
extern int alive_servers[MAX_SERVERS];
extern int total_servers;

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
void start_heartbeat();
void listen_for_heartbeat();
void send_heartbeat();
void handle_heartbeat_timeout();

#endif

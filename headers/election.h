#ifndef ELECTION_H
#define ELECTION_H

extern int read_sockfd, write_sockfd;
extern int heartbeat_sockfd;
extern int elected;
extern int is_participating;
extern int server_id;

struct election_msg {
  int elected;
  int id;
};

void setup_election_connection(int port);
void setup_election_socket(int port);

void start_election();
void send_election_message(struct election_msg election_msg);
struct election_msg receive_election_message();
void start_heartbeat();
void listen_for_heartbeat();
void send_heartbeat();
void handle_heartbeat_timeout();

#endif

#include "../headers/election.h"
#include "../headers/protocol.h"
#include <arpa/inet.h>
#include <errno.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
char server_ips[MAX_SERVERS][MAX_LINE_LENGTH];
int total_servers = 0;
int elected_server = -1;
int next_server = -1;
int is_participating = -1;
int server_id = -1;
int alive_servers[3] = {0};

int read_listen_sockfd;
int heartbeat_sockfd;
int elected_server;

void get_local_ip(char *buffer) {
  struct ifaddrs *ifaddr, *ifa;
  int family;

  if (getifaddrs(&ifaddr) == -1) {
    perror("getifaddrs");
    strcpy(buffer, "127.0.0.1"); // Fallback to localhost
    return;
  }

  for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
    if (ifa->ifa_addr == NULL)
      continue;

    family = ifa->ifa_addr->sa_family;

    if (family == AF_INET && strcmp(ifa->ifa_name, "lo") != 0) {
      // Exclude loopback ("lo") interface
      getnameinfo(ifa->ifa_addr, sizeof(struct sockaddr_in), buffer, NI_MAXHOST,
                  NULL, 0, NI_NUMERICHOST);
      break; // Use the first valid interface found
    }
  }

  freeifaddrs(ifaddr);
}

void read_server_config() {
  FILE *file = fopen(CONFIG_FILE_NAME, "r");
  if (!file) {
    perror("Failed to open server_config.csv");
    exit(EXIT_FAILURE);
  }

  char line[MAX_LINE_LENGTH];
  int i = 0;
  while (fgets(line, sizeof(line), file)) {
    strtok(line, "\n");
    strcpy(server_ips[i++], line);
  }
  total_servers = i;

  fprintf(stderr, "config readed\n");
  fclose(file);
}

void set_servers() {
  is_participating = -1;
  elected_server = -1;

  char local_ip[256];
  get_local_ip(local_ip);

  fprintf(stderr, "Local IP:%s\n", local_ip);

  for (int i = 0; i < total_servers; i++) {
    alive_servers[i] = 0;
    if (strcmp(local_ip, server_ips[i]) == 0) {
      server_id = i;
    }
  }

  next_server = (server_id + 1) % total_servers;

  fprintf(stderr, "servers setted\n");
  fprintf(stderr, "server_id:%d\n", server_id);
  fprintf(stderr, "next_server:%d\n", next_server);
}

void setup_election_socket(int port) {
  struct sockaddr_in addr;
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0) {
    perror("Failed to create socket");
    exit(EXIT_FAILURE);
  }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port);

  if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("Bind failed");
    exit(EXIT_FAILURE);
  }

  if (listen(sockfd, 5) < 0) {
    perror("Listen failed");
    exit(EXIT_FAILURE);
  }

  if (port == ELECTION_PORT)
    read_listen_sockfd = sockfd;
  if (port == HEARTBEAT_PORT)
    heartbeat_sockfd = sockfd;

  fprintf(stderr, "setup election sockfd\n");
}

void send_election_message(struct election_msg msg) {
  struct sockaddr_in addr;

  fprintf(stderr, "send message function\n");
  addr.sin_family = AF_INET;
  addr.sin_port = htons(ELECTION_PORT);
  inet_pton(AF_INET, server_ips[next_server], &addr.sin_addr);

  int sockfd;
  while (1) {
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
      perror("Socket creation failed");
      sleep(1);
      continue;
    }

    if (connect(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
      perror("Connection failed, retrying...");
      close(sockfd);
      sleep(1);
      continue;
    }
    break;
  }

  fprintf(stderr, "sending message\n");
  packet pkt = create_packet(S_ELECTION, 0, 0, "", 0);
  memcpy(pkt._payload, &msg, sizeof(struct election_msg));
  pkt.length = sizeof(struct election_msg);

  if (send_message(sockfd, pkt) != 0) {
    perror("Failed to send election message\n");
  }
  close(sockfd);
}

struct election_msg receive_election_message() {
  struct sockaddr_in addr;
  socklen_t addr_len = sizeof(addr);
  struct election_msg msg;
  int new_sock;

  fprintf(stderr, "rcv election message\n");
  new_sock = accept(read_listen_sockfd, (struct sockaddr *)&addr, &addr_len);
  if (new_sock < 0) {
    perror("Accept failed");
    msg.elected = -1;
    return msg;
  }

  packet response;

  fprintf(stderr, "rcv message\n");
  if (rcv_message(new_sock, S_ELECTION, 0, &response) != 0) {
    perror("Failed to receive election message\n");
    msg.elected = -1;
  }

  memcpy(&msg, response._payload, sizeof(struct election_msg));
  close(new_sock);
  return msg;
}

void start_election() {
  fprintf(stderr, "start election\n");
  struct election_msg msg;
  msg.id = server_id;
  msg.elected = -1;
  is_participating = 0;
  send_election_message(msg);
  fprintf(stderr, "start election sended\n");
}

void *handle_election() {
  struct election_msg msg = receive_election_message();

  fprintf(stderr, "handle election\n");

  if (msg.elected == 0 && server_id == msg.id) {
    fprintf(stderr, "Loop 2: Server %d elected\n", msg.id);
    return NULL;
  }
  if (server_id == msg.id && msg.elected == -1) {
    fprintf(stderr, "Loop 1: Server %d elected\n", msg.id);
    msg.elected = 0;
    msg.id = server_id;
    elected_server = server_id;
    is_participating = -1;
    send_election_message(msg);
    return NULL;
  }

  if (msg.elected != -1) {
    fprintf(stderr, "Server %d elected\n", msg.id);
    is_participating = -1;
    elected_server = msg.id;
    send_election_message(msg);
    return NULL;
  }

  if (server_id > msg.id && is_participating == -1) {
    msg.id = server_id;
    msg.elected = -1;
    is_participating = 0;
    send_election_message(msg);
    return NULL;
  }

  if (server_id < msg.id) {
    send_election_message(msg);
    return NULL;
  }

  return NULL;
}

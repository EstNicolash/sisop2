#include "../headers/election.h"
#include "../headers/server_handlers.h"

#include <unistd.h>

int server_setup(int port);
int server_accept_client(int sockfd);
void *client_handler(void *arg);
void *election_manager(void *arg);

pthread_mutex_t election_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t election_cond = PTHREAD_COND_INITIALIZER;

#define PORT 48487
int main() {

  // ELECTION
  read_server_config();
  setup_election_socket(ELECTION_PORT);
  pthread_t election_manager_thread;
  if (pthread_create(&election_manager_thread, NULL, election_manager, NULL) !=
      0) {
    perror("Thread creation failed\n");
    exit(EXIT_FAILURE);
  }

  pthread_mutex_lock(&election_mutex);
  while (server_id != elected_server) {
    pthread_cond_wait(&election_cond, &election_mutex);
  }
  pthread_mutex_unlock(&election_mutex);

  /*CLIENT*/
  int server_fd = server_setup(PORT);
  int client_propagation_read_fd = server_setup(PORT + 1);
  int client_propagation_write_fd = server_setup(PORT + 2);
  connection_map_init();

  while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int *client_fd = malloc(sizeof(int));
    *client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

    if (*client_fd < 0) {
      perror("Accept failed");
      free(client_fd);
      continue;
    }

    int *sockets = malloc(3 * sizeof(int));
    if (sockets == NULL) {
      perror("malloc failed");
      return -1;
    }

    sockets[0] = *client_fd;
    sockets[1] = client_propagation_read_fd;
    sockets[2] = client_propagation_write_fd;

    pthread_t tid;
    if (pthread_create(&tid, NULL, client_handler, sockets) != 0) {
      perror("Thread creation failed");
      close(*client_fd);
      free(client_fd);
    }

    pthread_detach(tid);
  }

  close(server_fd);
  return 0;
}

int server_setup(int port) {
  int sockfd;
  struct sockaddr_in serv_addr;

  // Create socket
  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("ERROR opening socket");
    return -1;
  }

  // Configure server address
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  memset(&(serv_addr.sin_zero), 0, 8);

  // Bind socket
  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR on binding");
    close(sockfd);
    return -1;
  }

  // Start listening
  printf("Start listening\n");
  if (listen(sockfd, 5) < 0) {
    perror("ERROR on listen");
    close(sockfd);
    return -1;
  }

  return sockfd;
}

void *election_manager(void *arg) {
  handle_election();
  pthread_mutex_lock(&election_mutex);
  pthread_cond_signal(&election_cond);
  pthread_mutex_unlock(&election_mutex);
  return NULL;
}

void *client_handler(void *arg) {

  int *sockets = (int *)arg;
  int sockfd = sockets[0];
  int propagation_r_listen_fd = sockets[1];
  int propagation_w_listen_fd = sockets[2];
  free(arg);

  struct sockaddr_in propagation_r_addr;
  socklen_t len = sizeof(propagation_r_addr);
  int propagation_read_sockfd = accept(
      propagation_r_listen_fd, (struct sockaddr *)&propagation_r_addr, &len);

  struct sockaddr_in propagation_w_addr;
  socklen_t len2 = sizeof(propagation_w_addr);
  int propagation_write_sockfd = accept(
      propagation_w_listen_fd, (struct sockaddr *)&propagation_w_addr, &len2);

  char user_id[MAX_FILENAME_SIZE] = {0};

  // Handle client identification
  if (server_handles_id(sockfd, user_id) < 0) {
    printf("Failed to identify client. Closing connection.\n");
    close(sockfd);
    return NULL;
  }

  printf("Client identified: %s\n", user_id);

  create_directory(user_id);

  if (add_connection(user_id, sockfd, propagation_read_sockfd,
                     propagation_write_sockfd) != 0) {
    printf("Client %s has reached the maximum connection limit. Closing "
           "connection.\n",
           user_id);
    close(sockfd);
    return NULL;
  }

  while (1) {

    packet received_packet;
    if (rcv_message(sockfd, ANYTHING, 0, &received_packet) != 0) {
      fprintf(stderr, "Failed to receive packet");
    };

    printf("Received request from client %s (type: %d):\n", user_id,
           received_packet.type);

    switch (received_packet.type) {
    case C_LIST_SERVER:
      server_handles_list_server(sockfd, user_id);
      break;

    case C_UPLOAD:
      server_handles_upload(sockfd, user_id);
      break;

    case C_DOWNLOAD:
      server_handles_download(sockfd, user_id);
      break;

    case C_GET_SYNC_DIR:
      printf("Sync Request: %s\n", user_id);
      server_handles_get_sync_dir(sockfd);
      break;

    case C_DELETE:
      server_handles_delete(sockfd, user_id);
      break;
    case C_EXIT:
      goto end;
      break;
    default:
      printf("Unknown command type: %d\n", received_packet.type);
      goto end;
      break;
    }
  }
end:

  remove_connection(user_id, sockfd);
  close(sockfd);
  printf("Connection with client %s closed.\n", user_id);
  return NULL;
}

#include "../headers/server_handlers.h"

int server_setup(int port);
int server_accept_client(int sockfd);
void *client_handler(void *arg);
#define PORT 55555
int main() {
  int server_fd = server_setup(PORT);

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

    pthread_t tid;
    if (pthread_create(&tid, NULL, client_handler, client_fd) != 0) {
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

void *client_handler(void *arg) {
  int sockfd = *((int *)arg);
  free(arg);

  packet received_packet;
  char user_id[MAX_FILENAME_SIZE] = {0};

  // Handle client identification
  if (server_handles_id(sockfd, user_id) < 0) {
    printf("Failed to identify client. Closing connection.\n");
    close(sockfd);
    return NULL;
  }

  printf("Client identified: %s\n", user_id);

  // Check and update connection count for this client
  // if (!update_connection_count(sockfd, user_id, 1)) {
  //  printf("Client %s has reached the maximum connection limit. Closing "
  //         "connection.\n",
  //         user_id);
  //  close(sockfd);
  //  return NULL;
  //}
  while (true)
    ;

  /*
  create_user_directory(user_id);

  while (true) {
    memset(&received_packet, 0, sizeof(received_packet));
    ssize_t bytes_read =
        read(sockfd, &received_packet, sizeof(received_packet));
    if (bytes_read <= 0) {
      if (bytes_read < 0)
        perror("Error reading from client socket");
      printf("Client %s disconnected.\n", user_id);
      break;
    }

    printf("Received request from client %s (type: %d):\n", user_id,
           received_packet.type);

    switch (received_packet.type) {
    case CMD_LIST_SERVER:
      server_handles_client_list_server(sockfd, user_id);
      break;

    case CMD_UPLOAD:
      server_handles_client_upload(sockfd, user_id, &received_packet);
      break;

    case CMD_DOWNLOAD:
      server_handles_client_download(sockfd, user_id, &received_packet);
      break;

    case CMD_GET_SYNC_DIR:
      server_handles_client_get_sync_dir(sockfd, user_id);
      break;

    case CMD_DELETE:
      server_handles_client_delete(sockfd, user_id, &received_packet);
      break;

    default:
      printf("Unknown command type: %d\n", received_packet.type);
      break;
    }
  }*/

  // update_connection_count(sockfd, user_id, -1);

  close(sockfd);
  printf("Connection with client %s closed.\n", user_id);
  return NULL;
}

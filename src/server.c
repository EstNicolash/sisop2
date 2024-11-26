#include "../headers/server_handlers.h"
#include <unistd.h>

int server_setup(int port);
int server_accept_client(int sockfd);
void *client_handler(void *arg);
#define PORT 48487
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
  client_count = 0;

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

  create_directory(user_id);

  // Check and update connection count for this client
  if (update_connection_count(user_id, 1) != 0) {
    printf("Client %s has reached the maximum connection limit. Closing "
           "connection.\n",
           user_id);
    close(sockfd);
    return NULL;
  }

  while (1) {

    packet received_packet;
    read(sockfd, &received_packet, sizeof(received_packet));

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

  update_connection_count(user_id, -1);

  close(sockfd);
  printf("Connection with client %s closed.\n", user_id);
  return NULL;
}

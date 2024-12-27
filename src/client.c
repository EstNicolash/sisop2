#include "../headers/client_threads.h"
#include "../headers/file_manager.h"
#include <arpa/inet.h> // For inet_pton, htons, sockaddr_in
#include <limits.h>
#include <netinet/in.h> // For sockaddr_in
#include <pthread.h>
#include <stdio.h>
#include <stdio.h>      // For printf, perror
#include <stdlib.h>     // For exit
#include <string.h>     // For memset
#include <sys/socket.h> // For socket, connect, AF_INET, SOCK_STREAM
#include <unistd.h>     // For close

#define BUFFER_SIZE 1024

#define DOWNLOAD_STR "download"
#define UPLOAD_STR "upload"
#define DELETE_STR "delete"
#define LIST_SERVER_STR "list_server"
#define LIST_CLIENT_STR "list_client"
#define GET_SYNC_DIR_STR "get_sync_dir"
#define EXIT_STR "exit"
// pthread_mutex_t client_sync_mutex = PTHREAD_MUTEX_INITIALIZER;
int client_connect(const char *server_ip, int port);

int main(int argc, char *argv[]) {
  int sockfd;
  char buffer[BUFFER_SIZE];
  char client_id[BUFFER_SIZE];
  char server_ip[BUFFER_SIZE];
  char server_port[BUFFER_SIZE];

  printf("Client start\n");

  strncpy(client_id, argv[1], BUFFER_SIZE);
  strncpy(server_ip, argv[2], BUFFER_SIZE);
  strncpy(server_port, argv[3], BUFFER_SIZE);

  printf("Client ID: %s\n", client_id);
  printf("Server IP: %s\n", server_ip);
  printf("Server Port: %s\n", server_port);

  int port = atoi(server_port);
  if (port <= 0) {
    fprintf(stderr, "Invalid port, using default port %d\n", port);
    exit(0);
  }

  sockfd = client_connect(server_ip, port);
  if (sockfd < 0) {
    exit(0);
  }

  if (client_send_id(sockfd, client_id) != 0) {
    fprintf(stderr, "Error sending ID\n");
    close(sockfd);
    exit(0);
  }
  const char dir[MAX_FILENAME_SIZE] = SYNC_DIR;
  create_directory(dir);

  pthread_t monitor_thread;
  if (pthread_create(&monitor_thread, NULL, inotify_thread, &sockfd) != 0) {
    perror("Failed to create thread");
    close(sockfd);
    return EXIT_FAILURE;
  }

  pthread_t sync_thread;
  if (pthread_create(&sync_thread, NULL, sync_dir_thread, &sockfd) != 0) {
    perror("Failed to create sync thread");
    close(sockfd);
    // pthread_mutex_destroy(&client_sync_mutex);
    return EXIT_FAILURE;
  }

  pthread_t msg_thread;
  if (pthread_create(&msg_thread, NULL, messages_thread, &sockfd) != 0) {
    perror("Failed to create sync thread");
    close(sockfd);
    // pthread_mutex_destroy(&client_sync_mutex);
    return EXIT_FAILURE;
  }
  while (1) {
    printf("Enter the message: \n");
    bzero(buffer, BUFFER_SIZE);
    fgets(buffer, BUFFER_SIZE, stdin);

    buffer[strcspn(buffer, "\n")] = 0;

    char *command = strtok(buffer, " ");
    char *filename = strtok(NULL, " ");

    // pthread_mutex_lock(&client_sync_mutex);
    if (command && filename) {
      char filename_info[MAX_PAYLOAD_SIZE];
      strncpy(filename_info, filename, MAX_PAYLOAD_SIZE);

      if (strcmp(command, DOWNLOAD_STR) == 0) {
        msg_queue_insert(C_DOWNLOAD, filename_info);

      } else if (strcmp(command, UPLOAD_STR) == 0) {
        msg_queue_insert(C_UPLOAD, filename_info);

      } else if (strcmp(command, DELETE_STR) == 0) {
        msg_queue_insert(C_DELETE, filename_info);
      }
    } else if (command) {

      char msg[MAX_PAYLOAD_SIZE] = {""};

      if (strcmp(command, LIST_CLIENT_STR) == 0) {
        client_list_client();
      } else if (strcmp(command, LIST_SERVER_STR) == 0) {
        msg_queue_insert(C_LIST_SERVER, msg);
      } else if (strcmp(command, GET_SYNC_DIR_STR) == 0) {
        msg_queue_insert(C_GET_SYNC_DIR, msg);

      } else if (strcmp(command, EXIT_STR) == 0) {
        client_exit(sockfd);
        is_sync_running = -1;
        is_inotify_running = -1;
        is_messages_running = -1;
        break;
      }
    }
    // pthread_mutex_unlock(&client_sync_mutex);
  }
  if (pthread_join(sync_thread, NULL) != 0) {
    perror("Failed to join sync thread");
  }

  if (pthread_join(monitor_thread, NULL) != 0) {
    perror("Failed to join monitor thread");
  }

  if (pthread_join(msg_thread, NULL) != 0) {
    perror("Failed to join message thread");
  }

  // pthread_mutex_destroy(&client_sync_mutex);
  close(sockfd);
  printf("Client stop\n");
  return 0;
}

int client_connect(const char *server_ip, int port) {
  int sockfd;
  struct sockaddr_in serv_addr;

  if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("ERROR opening socket");
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
    perror("Invalid IP address or address not supported");
    close(sockfd);
    return -1;
  }

  printf("Attempting connection\n");
  if (connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
    perror("ERROR connecting");
    close(sockfd);
    return -1;
  }

  printf("Connected\n");
  return sockfd;
}

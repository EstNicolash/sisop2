#include "../headers/client_threads.h"
#include "../headers/file_manager.h"
#include <arpa/inet.h> // For inet_pton, htons, sockaddr_in
#include <libgen.h>
#include <limits.h>
#include <netinet/in.h> // For sockaddr_in
#include <pthread.h>
#include <stdio.h>
#include <stdio.h>      // For printf, perror
#include <stdlib.h>     // For exit
#include <string.h>     // For memset
#include <sys/socket.h> // For socket, connect, AF_INET, SOCK_STREAM
#include <unistd.h>     // For close
                        //
#include <libgen.h>
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
  char buffer[BUFFER_SIZE];
  char server_port[BUFFER_SIZE];

  printf("Client start\n");

  strncpy(client_id, argv[1], BUFFER_SIZE);
  strncpy(server_ip, argv[2], 256);
  strncpy(server_port, argv[3], BUFFER_SIZE);

  printf("Client ID: %s\n", client_id);
  printf("Server IP: %s\n", server_ip);
  printf("Server Port: %s\n", server_port);

  port = atoi(server_port);
  if (port <= 0) {
    fprintf(stderr, "Invalid port, using default port %d\n", port);
    exit(0);
  }

  sockfd = client_connect(server_ip, port);
  if (sockfd < 0) {
    exit(0);
  }

  prop_read_sockfd = client_connect(server_ip, port + 1);
  if (prop_read_sockfd < 0) {
    exit(0);
  }

  prop_write_sockfd = client_connect(server_ip, port + 2);
  if (prop_write_sockfd < 0) {
    exit(0);
  }

  printf("SOCKETS: %d,%d,%d\n", sockfd, prop_read_sockfd, prop_write_sockfd);

  if (client_send_id(sockfd, client_id) != 0) {
    fprintf(stderr, "Error sending ID\n");
    close(sockfd);
    exit(0);
  }
  const char dir[MAX_FILENAME_SIZE] = SYNC_DIR;
  create_directory(dir);

  pthread_t hrtbt_thread;
  if (pthread_create(&hrtbt_thread, NULL, heartbeat_thread, NULL) != 0) {
    perror("Failed to create thread");
    close(sockfd);
    return EXIT_FAILURE;
  }

  pthread_t monitor_thread;
  if (pthread_create(&monitor_thread, NULL, inotify_thread, NULL) != 0) {
    perror("Failed to create thread");
    close(sockfd);
    return EXIT_FAILURE;
  }

  pthread_t sync_thread;
  if (pthread_create(&sync_thread, NULL, sync_dir_thread, NULL) != 0) {
    perror("Failed to create sync thread");
    close(sockfd);
    // pthread_mutex_destroy(&client_sync_mutex);
    return EXIT_FAILURE;
  }

  pthread_t msg_thread;
  if (pthread_create(&msg_thread, NULL, messages_thread, NULL) != 0) {
    perror("Failed to create sync thread");
    close(sockfd);
    // pthread_mutex_destroy(&client_sync_mutex);
    return EXIT_FAILURE;
  }

  pthread_t prop_thread;
  if (pthread_create(&prop_thread, NULL, rcv_propagation_thread, NULL) != 0) {
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
    char msg[MAX_PAYLOAD_SIZE] = {""};

    if (strcmp(command, EXIT_STR) == 0) {
      client_exit(sockfd);
      is_sync_running = -1;
      is_inotify_running = -1;
      is_messages_running = -1;
      is_rcv_propagation_running = -1;
      break;
    }

    if (strcmp(command, LIST_CLIENT_STR) == 0) {
      client_list_client();
      continue;
    }
    if (strcmp(command, LIST_SERVER_STR) == 0) {
      msg_queue_insert(C_LIST_SERVER, msg);
      continue;
    }
    if (strcmp(command, GET_SYNC_DIR_STR) == 0) {
      msg_queue_insert(C_GET_SYNC_DIR, msg);
      continue;
    }

    if (!filename)
      continue;

    char filename_info[MAX_PAYLOAD_SIZE];
    strncpy(filename_info, filename, MAX_PAYLOAD_SIZE);

    if (strcmp(command, DOWNLOAD_STR) == 0) {
      msg_queue_insert(C_DOWNLOAD, filename_info);
      continue;
    }

    if (strcmp(command, UPLOAD_STR) == 0) {
      copy_file(filename, SYNC_DIR);
      msg_queue_insert(C_UPLOAD, filename_info);
      continue;
    }

    if (strcmp(command, DELETE_STR) == 0) {
      fprintf(stderr, "DELETE CMD\n");
      char file_path[MAX_PAYLOAD_SIZE * 2];
      snprintf(file_path, sizeof(file_path), "%s/%s", SYNC_DIR, filename);
      delete_file(file_path);
      msg_queue_insert(C_DELETE, filename_info);
      continue;
    }
  }

  fprintf(stderr, "0...\n");
  if (pthread_join(sync_thread, NULL) != 0) {
    perror("Failed to join sync thread");
  }

  fprintf(stderr, "1...\n");
  if (pthread_join(hrtbt_thread, NULL) != 0) {
    perror("Failed to join sync thread");
  }

  fprintf(stderr, "2...\n");
  if (pthread_join(monitor_thread, NULL) != 0) {
    perror("Failed to join monitor thread");
  }

  fprintf(stderr, "3...\n");
  if (pthread_join(msg_thread, NULL) != 0) {
    perror("Failed to join message thread");
  }

  fprintf(stderr, "4...\n");
  if (pthread_join(prop_thread, NULL) != 0) {
    perror("Failed to join message thread");
  }

  fprintf(stderr, "5...\n");

  close(sockfd);
  close(prop_write_sockfd);
  close(prop_read_sockfd);
  printf("Client stop\n");
  return 0;
}

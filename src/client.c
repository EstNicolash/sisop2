#include "../headers/client_commands.h"
#include "../headers/file_manager.h"
#include <arpa/inet.h> // For inet_pton, htons, sockaddr_in
#include <errno.h>
#include <limits.h>
#include <netinet/in.h> // For sockaddr_in
#include <pthread.h>
#include <stdio.h>
#include <stdio.h>  // For printf, perror
#include <stdlib.h> // For exit
#include <string.h> // For memset
#include <sys/inotify.h>
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
#define EVENT_SIZE (sizeof(struct inotify_event))
#define EVENT_BUF_LEN (1024 * (EVENT_SIZE + 16))
int sync_active = 1;
pthread_mutex_t client_sync_mutex; // Mutex for synchronization
int client_connect(const char *server_ip, int port);
void *sync_thread_function(void *arg);
// void *inotify_handler(void *arg);

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

  // Initialize the mutex
  pthread_mutex_init(&client_sync_mutex, NULL);

  // Create the sync thread
  /*
  pthread_t inotify_thread;
  if (pthread_create(&inotify_thread, NULL, inotify_handler, &sockfd) != 0) {
    perror("Failed to create inotify handler thread");
    pthread_mutex_destroy(&client_sync_mutex);
    close(sockfd);
    return EXIT_FAILURE;
  }*/
  pthread_t sync_thread;
  /*
    if (pthread_create(&sync_thread, NULL, sync_thread_function, &sockfd) != 0)
    { perror("Failed to create sync thread"); close(sockfd);
      pthread_mutex_destroy(&client_sync_mutex);
      return EXIT_FAILURE;
    }*/
  while (1) {
    printf("Enter the message: \n");
    bzero(buffer, BUFFER_SIZE);
    fgets(buffer, BUFFER_SIZE, stdin);

    buffer[strcspn(buffer, "\n")] = 0;

    char *command = strtok(buffer, " ");
    char *filename = strtok(NULL, " ");

    pthread_mutex_lock(&client_sync_mutex);
    if (command && filename) {
      if (strcmp(command, DOWNLOAD_STR) == 0) {
        char cwd[MAX_FILENAME_SIZE];

        if (getcwd(cwd, sizeof(cwd)) != NULL) {
          printf("Current working directory: %s\n", cwd);
        } else {
          perror("getcwd() error");
        }
        client_download_file(sockfd, filename, cwd);

      } else if (strcmp(command, UPLOAD_STR) == 0) {
        client_upload_file(sockfd, filename);
      } else if (strcmp(command, DELETE_STR) == 0) {
        client_delete_file(sockfd, filename);
      }
    } else if (command) {
      if (strcmp(command, LIST_CLIENT_STR) == 0) {
        client_list_client(sockfd);
      } else if (strcmp(command, LIST_SERVER_STR) == 0) {
        client_list_server(sockfd);
      } else if (strcmp(command, GET_SYNC_DIR_STR) == 0) {
        get_sync_dir(sockfd);
      } else if (strcmp(command, EXIT_STR) == 0) {
        client_exit(sockfd);
        sync_active = 0;
        break;
      }
    }
    pthread_mutex_unlock(&client_sync_mutex);
  }
  // if (pthread_join(sync_thread, NULL) != 0) {
  //   perror("Failed to join sync thread");
  // }
  pthread_mutex_destroy(&client_sync_mutex);
  close(sockfd);
  printf("Client stop\n");
  return 0;
}
// Thread function to periodically sync directories
void *sync_thread_function(void *arg) {
  int sockfd = *(int *)arg;
  while (sync_active == 1) {
    pthread_mutex_lock(&client_sync_mutex);
    // printf("Starting sync...\n");
    if (get_sync_dir(sockfd) != 0) {
      fprintf(stderr, "Error syncing directories\n");
    }
    pthread_mutex_unlock(&client_sync_mutex);
    sleep(3);
  }
  printf("Sync thread exiting...\n");
  return NULL;
}

/*
void *inotify_handler(void *arg) {
  int sockfd = *(int *)arg;        // File descriptor for server communication
  int inotify_fd = inotify_init(); // Initialize inotify instance

  if (inotify_fd < 0) {
    perror("inotify_init failed");
    return NULL;
  }

  int watch_fd = inotify_add_watch(inotify_fd, SYNC_DIR,
                                   IN_CREATE | IN_MODIFY | IN_DELETE);
  if (watch_fd < 0) {
    perror("inotify_add_watch failed");
    close(inotify_fd);
    return NULL;
  }

  char buffer[EVENT_BUF_LEN];
  while (1) {
    int length = read(inotify_fd, buffer, EVENT_BUF_LEN);
    if (length < 0) {
      perror("read error");
      break;
    }

    int i = 0;
    while (i < length) {
      struct inotify_event *event = (struct inotify_event *)&buffer[i];

      if (event->len) {
        pthread_mutex_lock(&client_sync_mutex);

        if (event->mask & IN_CREATE) {
          // printf("File created: %s\n", event->name);
          client_upload_file(sockfd, event->name);
        }
        if (event->mask & IN_MODIFY) {
          // printf("File modified: %s\n", event->name);
          client_upload_file(sockfd, event->name);
        }
        if (event->mask & IN_DELETE) {
          // printf("File deleted: %s\n", event->name);
          client_delete_file(sockfd, event->name);
        }

        pthread_mutex_unlock(&client_sync_mutex);
      }
      i += EVENT_SIZE + event->len;
    }
  }

  inotify_rm_watch(inotify_fd, watch_fd);
  close(inotify_fd);
  return NULL;
}*/

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

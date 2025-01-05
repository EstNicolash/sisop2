#include "../headers/election.h"
#include "../headers/server_handlers.h"

#include <unistd.h>

int server_setup(int port);
int server_accept_client(int sockfd);
void *client_handler(void *arg);
void *election_manager(void *arg);
void *listen_for_replica(void *arg);
void *receive_user_dir(int sockfd, const char user_id[MAX_FILENAME_SIZE]);

#define PORT2 48487
#define PORT 48480
int main() {

  // ELECTION
  read_server_config();
  set_servers();
  setup_election_socket(ELECTION_PORT);
  start_election();
  pthread_t election_manager_thread;
  if (pthread_create(&election_manager_thread, NULL, election_manager, NULL) !=
      0) {
    perror("Thread creation failed\n");
    exit(EXIT_FAILURE);
  }

  pthread_t listen_hearbeat_thread;
  if (pthread_create(&listen_hearbeat_thread, NULL, listen_for_heartbeat,
                     NULL) != 0) {
    perror("Thread creation failed\n");
    exit(EXIT_FAILURE);
  }

  pthread_t send_heartbeat_thread;
  if (pthread_create(&send_heartbeat_thread, NULL, send_heartbeat, NULL) != 0) {
    perror("Thread creation failed\n");
    exit(EXIT_FAILURE);
  }

  pthread_mutex_lock(&election_mutex);
  while (server_id != elected_server) {
    pthread_cond_wait(&election_cond, &election_mutex);
  }
  pthread_mutex_unlock(&election_mutex);

  pthread_t replica_thread;
  if (pthread_create(&replica_thread, NULL, listen_for_replica, &next_server) != 0) {
    perror("Thread creation failed\n");
    exit(EXIT_FAILURE);
  }

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
  while (1) {
    handle_election();
    pthread_mutex_lock(&election_mutex);
    pthread_cond_signal(&election_cond);
    pthread_mutex_unlock(&election_mutex);
  }
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

void *listen_for_replica(void *arg) {
  int replica_fd = server_setup(REPLICA_PORT);
  int *client_fd = malloc(sizeof(int));
  struct sockaddr_in client_addr;
  socklen_t addr_len = sizeof(client_addr);

  while (1) {
    *client_fd = accept(replica_fd, (struct sockaddr *)&client_addr, &addr_len);
    if (client_fd < 0) {
      perror("Heartbeat accept failed");
      continue;
    }

    pthread_t tid;
    if (pthread_create(&tid, NULL, replica_handler, client_fd) != 0) {
      perror("Thread creation failed");
      close(*client_fd);
      free(client_fd);
    }

    pthread_detach(tid);
  }

  fprintf(stderr, "rcv heardbeat\n");
  close(replica_fd); // Cleanup socket
  return NULL;
}

void *replica_handler(void *arg) {
  int *sockfd = (int *)arg;
  char user_id[MAX_FILENAME_SIZE] = {0};

  printf("Client replica_handler\n");

  if (server_handles_id(*sockfd, user_id) > 0){
    receive_user_dir(*sockfd, user_id);
  }

  printf("Client replica_handler exit\n");

  return NULL;
}

void *receive_user_dir(int sockfd, const char user_id[MAX_FILENAME_SIZE]){
  create_directory(user_id);

  int server_file_count = 0;
  FileInfo *server_files = receive_file_list(sockfd, &server_file_count);

  if (server_files == NULL) {
    perror("error receiving files list from server\n");
    return NULL;
  }

  print_file_list(server_files, server_file_count);

  char local_ip[MAX_FILENAME_SIZE] = {0};
  get_local_ip(local_ip);

  // Local file LISt
  int local_file_count = 0;
  FileInfo *local_files = list_files(user_id, &local_file_count);

  FileInfo *to_delete_files = malloc(local_file_count * sizeof(FileInfo));
  FileInfo *to_download_files = malloc(server_file_count * sizeof(FileInfo));
  if (!to_delete_files || !to_download_files) {
    perror("Memory allocation failed\n");
    return NULL;
  }

  int to_delete_count = 0;
  int to_download_count = 0;

  // Identify files to download
  for (int i = 0; i < server_file_count; i++) {
    if (strcmp(server_files[i].original_ip, local_ip) != 0) {
      int found = 0;
      for (int j = 0; j < local_file_count; j++) {
        if (strcmp(server_files[i].filename, local_files[j].filename) == 0) {
          found = 1;

          if (strcmp((char *)server_files[i].md5_checksum,
                    (char *)local_files[j].md5_checksum) != 0) {

            to_download_files[to_download_count++] = server_files[i];
          }
          break;
        }
      }
      if (!found) {
        to_download_files[to_download_count++] = server_files[i];
      }
    }
    else{
      break;
    }
  }

  // Identify files to delete
  for (int i = 0; i < local_file_count; i++) {
    int found = 0;
    for (int j = 0; j < server_file_count; j++) {
      if (strcmp(local_files[i].filename, server_files[j].filename) == 0) {
        found = 1;
        break;
      }
    }
    if (!found) {
      to_delete_files[to_delete_count++] = local_files[i];
    }
  }

  free(local_files);
  free(server_files);

  for (int i = 0; i < to_delete_count; i++) {
    char file_path[MAX_PAYLOAD_SIZE * 2];
    snprintf(file_path, sizeof(file_path), "%s/%s", user_id,
             to_delete_files[i].filename);
    delete_file(file_path);
  }

  for (int i = 0; i < to_download_count; i++) {
    //client_download_file(sockfd, to_download_files[i].filename, user_id);
  }

  free(to_delete_files);
  free(to_download_files);
  return NULL;
}
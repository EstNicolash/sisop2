#include "../headers/server_handlers.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

Client manage_clients_connections[MAX_CLIENTS] = {0};
int client_count = 0;
pthread_mutex_t manage_clients_mutex = PTHREAD_MUTEX_INITIALIZER;

int server_handles_id(int sockfd, char user_id[MAX_FILENAME_SIZE]) {

  packet rcv_pkt;

  if (rcv_message(sockfd, C_SEND_ID, 0, &rcv_pkt) != 0)
    return -1;

  strncpy(user_id, rcv_pkt._payload, MAX_FILENAME_SIZE);

  packet pkt = create_packet(OK, 0, 0, "ok", 2);
  send_message(sockfd, pkt);
  return 1;
}

int server_handles_upload(int sockfd, const char user_id[MAX_FILENAME_SIZE]) {
  packet ack = create_packet(OK, C_UPLOAD, 0, "ok", 2);
  send_message(sockfd, ack);
  FileInfo metadada;
  uint32_t size = 0;
  char *file_buffer = receive_file(sockfd, &size, &metadada);

  save_file(metadada.filename, user_id, file_buffer, size);

  free(file_buffer);

  return 1;
}

int server_handles_download(int sockfd, const char user_id[MAX_FILENAME_SIZE]) {

  packet rcv_pkt;
  if (rcv_message(sockfd, C_DOWNLOAD, 1, &rcv_pkt) != 0) {
    perror("Error receiving filename\n");
    return -1;
  }

  char file_path[MAX_PAYLOAD_SIZE * 2];
  snprintf(file_path, sizeof(file_path), "%s/%s", user_id, rcv_pkt._payload);

  if (file_exists(file_path) != 0) {
    perror("File doesn't exist\n");

    packet error_pkt = create_packet(ERROR, 0, 0, "no", 2);
    send_message(sockfd, error_pkt);
    return -1;
  }

  packet ack_pkt = create_packet(OK, C_DOWNLOAD, 0, "ok", 2);

  if (send_message(sockfd, ack_pkt) != 0) {
    perror("Error sendinck ack\n");
    return -1;
  }

  return send_file(sockfd, file_path);
}

int server_handles_list_server(int sockfd,
                               const char user_id[MAX_FILENAME_SIZE]) {

  return send_file_list(sockfd, user_id);
}

int update_connection_count(const char *user_id, int delta) {
  pthread_mutex_lock(&manage_clients_mutex);

  for (int i = 0; i < client_count; i++) {
    if (strcmp(manage_clients_connections[i].user_id, user_id) == 0) {
      if (delta > 0 && manage_clients_connections[i].connection_count >=
                           MAX_CONNECTIONS_PER_CLIENT) {
        pthread_mutex_unlock(&manage_clients_mutex);
        return -1;
      }

      manage_clients_connections[i].connection_count += delta;

      if (manage_clients_connections[i].connection_count <= 0) {
        manage_clients_connections[i] =
            manage_clients_connections[--client_count];
      }

      pthread_mutex_unlock(&manage_clients_mutex);
      return 0;
    }
  }

  if (delta > 0 && client_count < MAX_CLIENTS) {
    strcpy(manage_clients_connections[client_count].user_id, user_id);
    manage_clients_connections[client_count].connection_count = delta;
    client_count++;
    pthread_mutex_unlock(&manage_clients_mutex);
    return 0;
  }

  pthread_mutex_unlock(&manage_clients_mutex);
  return -1;
}

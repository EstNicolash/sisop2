#include "../headers/server_handlers.h"
#include "../headers/connection_map.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

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

int server_handles_delete(int sockfd, const char user_id[MAX_FILENAME_SIZE]) {

  packet delete_pkt;

  if (rcv_message(sockfd, C_DELETE, 0, &delete_pkt) != 0) {
    perror("Failed to receive delete_msg\n");
    return -1;
  }

  char file_path[MAX_PAYLOAD_SIZE * 2];
  snprintf(file_path, sizeof(file_path), "%s/%s", user_id, delete_pkt._payload);

  if (file_exists(file_path) != 0) {
    perror("File doesn't exist\n");
    packet fail = create_packet(ERROR, C_DELETE, 0, "error", 5);
    send_message(sockfd, fail);
    return -1;
  }

  delete_file(file_path);

  packet ack = create_packet(OK, C_DELETE, 0, "ok", 2);

  if (send_message(sockfd, ack) != 0) {
    perror("error sending ack\n");
    return -1;
  }

  return 0;
}

int server_handles_get_sync_dir(int sockfd) {

  packet ack = create_packet(OK, C_GET_SYNC_DIR, 0, "ok", 2);
  if (send_message(sockfd, ack) != 0) {
    perror("Error sending ack get_sync\n");
    return -1;
  }

  // printf("Sync dir acked");
  return 0;
}

int add_connection(const char user_id[MAX_FILENAME_SIZE], int normal_sockfd,
                   int propagation_sockfd) {

  if (connection_map_count_user_connections(user_id) >= 2)
    return -1;

  connection_map_insert(user_id, normal_sockfd, propagation_sockfd);

  return 0;
}

int remove_connection(const char user_id[MAX_FILENAME_SIZE],
                      int normal_sockfd) {

  connection_map_delete(user_id, normal_sockfd);

  return 0;
}

void map_init() { connection_map_init(); }

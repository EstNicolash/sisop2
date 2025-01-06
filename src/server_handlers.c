#include "../headers/server_handlers.h"
#include "../headers/connection_map.h"
#include "../headers/election.h"
#include "../headers/replica.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

int server_handles_id(int sockfd, char user_id[MAX_FILENAME_SIZE]) {

  packet rcv_pkt;

  if (rcv_message(sockfd, C_SEND_ID, 0, &rcv_pkt) != 0)
    return -1;

  strncpy(user_id, rcv_pkt._payload, MAX_FILENAME_SIZE);

  packet pkt = create_packet(OK, 0, 0, server_ips[next_server],
                             strnlen(server_ips[next_server], 256));
  send_message(sockfd, pkt);
  return 1;
}

int server_handles_upload(int sockfd, const char user_id[MAX_FILENAME_SIZE]) {

  // Send ACK start
  packet ack = create_packet(OK, C_UPLOAD, 0, "ok", 2);
  send_message(sockfd, ack);

  FileInfo metadata;
  uint32_t size = 0;
  char *file_buffer = receive_file(sockfd, &size, &metadata);

  save_file(metadata.filename, user_id, file_buffer, size);

  if (propagate_to_client(sockfd, user_id, metadata.filename) != 0) {
    fprintf(stderr, "Error propagating to client\n");
  }
  
  for(int i =0; i < server_id; i++){
  	propagate_to_backup(replica_sockets[i], user_id, metadata.filename);
  }
  

  free(file_buffer);

  // SEND ACK END
  ack = create_packet(OK, C_UPLOAD, 0, "ok", 2);
  send_message(sockfd, ack);
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
  propagate_delete(sockfd, user_id, delete_pkt._payload);
  for(int i = 0; i < server_id; i++){
  	propagate_delete_to_backup(replica_sockets[i], user_id, delete_pkt._payload);
  }
  

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

int propagate_to_client(int sockfd, const char user_id[MAX_FILENAME_SIZE],
                        const char filename[MAX_FILENAME_SIZE]) {

  packet msg = create_packet(S_PROPAGATE, 0, 0, "upload", 6);

  ConnectionInfo *info = malloc(sizeof(ConnectionInfo));

  info = connection_map_search_other(user_id, sockfd);

  if (info == NULL) {
    fprintf(stderr, "No connection\n");
    return -1;
  }

  // fprintf(stderr, "Propagate from: %d", sockfd);
  // fprintf(stderr, "Popagate to: %d,%d\n", info->normal_sockfd,
  //        info->propagation_sockfd);

  fprintf(stderr, "Send propagation notification\n");
  if (send_message(info->propagation_read_sockfd, msg) != 0) {
    perror("Error sending propagate msg\n");
    return -1;
  }
  fprintf(stderr, "Receiving ok\n");
  // rcv Ack client command
  if (rcv_message(info->propagation_write_sockfd, OK, S_PROPAGATE, &msg) != 0) {
    perror("Error rcv_message ack propagate\n");
    return -1;
  }

  fprintf(stderr, "Sending metadata\n");
  char file_path[MAX_PAYLOAD_SIZE * 2];
  snprintf(file_path, sizeof(file_path), "%s/%s", user_id, filename);

  if (send_metadata(info->propagation_write_sockfd, file_path) != 0) {
    fprintf(stderr, "error or equal checksum\n");
    return -1;
  }

  fprintf(stderr, "Receiving ACK\n");
  // Ack receive
  if (rcv_message(info->propagation_write_sockfd, OK, S_PROPAGATE, &msg) != 0) {
    perror("Error rcv_message ack propagate or checksum or no file\n");
    return -1;
  }

  fprintf(stderr, "Teste 5\n");
  return send_file(info->propagation_write_sockfd, file_path);
}

int propagate_delete(int sockfd, const char user_id[MAX_FILENAME_SIZE],
                     const char filename[MAX_FILENAME_SIZE]) {

  packet msg = create_packet(S_PROPAGATE, 0, 0, "delete", 6);

  ConnectionInfo *info = malloc(sizeof(ConnectionInfo));

  info = connection_map_search_other(user_id, sockfd);

  if (info == NULL) {
    fprintf(stderr, "No connection\n");
    return -1;
  }

  // fprintf(stderr, "Propagate from: %d", sockfd);
  // fprintf(stderr, "Popagate to: %d,%d\n", info->normal_sockfd,
  //        info->propagation_sockfd);

  if (send_message(info->propagation_read_sockfd, msg) != 0) {
    perror("Error sending propagate msg\n");
    return -1;
  }
  fprintf(stderr, "Teste 1\n");
  // rcv Ack client command
  if (rcv_message(info->propagation_write_sockfd, OK, S_PROPAGATE, &msg) != 0) {
    perror("Error rcv_message ack propagate\n");
    return -1;
  }

  fprintf(stderr, "Teste 2\n");
  // char file_path[MAX_PAYLOAD_SIZE * 2];
  // snprintf(file_path, sizeof(file_path), "%s/%s", user_id, filename);
  packet to_delete =
      create_packet(S_PROPAGATE, C_DELETE, 0, filename, strlen(filename));
  if (send_message(info->propagation_write_sockfd, to_delete) != 0) {
    fprintf(stderr, "error or equal checksum\n");
    return -1;
  }

  fprintf(stderr, "Teste 4\n");
  // Ack receive
  if (rcv_message(info->propagation_write_sockfd, OK, S_PROPAGATE, &msg) != 0) {
    perror("Error rcv_message ack propagate or checksum or no file\n");
    return -1;
  }

  fprintf(stderr, "Teste 5\n");
  return 0;
}

int add_connection(const char user_id[MAX_FILENAME_SIZE], int normal_sockfd,
                   int propagation_r_sockfd, int propagation_w_sockfd) {

  // fprintf(stderr, "Sockets: %d,%d\n", normal_sockfd, propagation_sockfd);
  if (connection_map_count_user_connections(user_id) >= 2)
    return -1;

  connection_map_insert(user_id, normal_sockfd, propagation_r_sockfd,
                        propagation_w_sockfd);

  return 0;
}

int remove_connection(const char user_id[MAX_FILENAME_SIZE],
                      int normal_sockfd) {

  connection_map_delete(user_id, normal_sockfd);

  return 0;
}

void map_init() { connection_map_init(); }

#include "../headers/protocol.h"
#include <fcntl.h>
#include <libgen.h>
#include <stdint.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
packet create_packet(uint16_t type, uint16_t seqn, uint32_t total_size,
                     const void *_payload, size_t payload_size) {
  packet pkt;
  pkt.type = type;
  pkt.seqn = seqn;
  pkt.total_size = total_size;

  memset(pkt._payload, 0, MAX_PAYLOAD_SIZE);
  memcpy(pkt._payload, _payload, payload_size);

  pkt.length = (uint16_t)payload_size;

  return pkt;
}

int send_message(int sockfd, packet pkt) {
  ssize_t bytes_sent = write(sockfd, &pkt, sizeof(packet));
  if (bytes_sent != sizeof(packet)) {
    perror("Failed to send packet");
    return -1;
  }
  return 0;
}
int rcv_message(int sockfd, uint16_t type, uint16_t seqn, packet *rcv_pkt) {
  char buffer[MAX_PAYLOAD_SIZE];
  ssize_t bytes_received = read(sockfd, buffer, sizeof(packet));
  ssize_t received_size = 0;
  memcpy(rcv_pkt + received_size, buffer, bytes_received);
  ssize_t received_size = bytes_received;

  
   while (received_size < sizeof(packet)) {
    // Receive raw bytes directly into the buffer
    ssize_t bytes_received = recv(sockfd, buffer, MAX_PAYLOAD_SIZE, 0);

    if (bytes_received < 0) {
      perror("Error receiving data");
     // free(file_data);
      return -1;
    }

    if (bytes_received == 0) {
      // Connection closed by the sender
      printf("Connection closed by sender\n");
      break;
    }

    // Store the received bytes into the file data buffer
    memcpy(rcv_pkt + received_size, buffer, bytes_received);
    received_size += bytes_received;
  }


  // Validate the packet's type and sequence number
  if (rcv_pkt->type != type || rcv_pkt->seqn != seqn) {
    fprintf(stderr, "Packet verification failed (type or seqn mismatch)\n");
    printf("Expected type: %d, seqn: %d. Received type: %d, seqn: %d\n", type,
           seqn, rcv_pkt->type, rcv_pkt->seqn);
    return -1;
  }

  return 0;
}

int send_file(int sockfd, const char file_name[MAX_FILENAME_SIZE]) {
  //  fprintf(stderr, "Send_file(%d, %s)\n", sockfd, file_name);

  packet error_pkt = create_packet(ERROR, ERROR, 0, "error", 4);
  int file_fd = open(file_name, O_RDONLY);
  if (file_fd < 0) {
    perror("File opening failed");
    send_message(sockfd, error_pkt);
    return -1;
  }

  struct stat file_stat;
  if (fstat(file_fd, &file_stat) != 0) {
    perror("Failed to get file stats");
    send_message(sockfd, error_pkt);
    close(file_fd);
    return -1;
  }

  uint32_t total_size = (uint32_t)file_stat.st_size;

  if (total_size == 0) {
    printf("Error: File is empty or could not determine size: %s\n", file_name);
    send_message(sockfd, error_pkt);
    close(file_fd);
    return -1;
  }

  FileInfo file_info = get_file_info(file_name);
  file_info.file_size = total_size;
  char *base_name = basename(file_info.filename);
  strncpy(file_info.filename, base_name, MAX_FILENAME_SIZE);

  packet metadata_pkt = create_packet(METADATA, 0, total_size, "file", 4);
  memcpy(metadata_pkt._payload, &file_info, sizeof(FileInfo));
  metadata_pkt.length = sizeof(FileInfo);

  // Send metadata
  if (send_message(sockfd, metadata_pkt) != 0) {
    perror("Failed to send file metadata");
    close(file_fd);
    return -1;
  }

  printf("Metadata sended\n");
  size_t read_size;
  char buffer[MAX_PAYLOAD_SIZE];

  ssize_t total = 0;
  while ((read_size = read(file_fd, buffer, MAX_PAYLOAD_SIZE)) > 0) {
    // Send the raw bytes directly, without creating a packet structure
    ssize_t bytes_sent = send(sockfd, buffer, read_size, 0);

    if (bytes_sent < 0) {
      perror("Send failed during file transfer");
      close(file_fd);
      return -1;
    }

    total += bytes_sent;
    // printf("Sent %ld bytes Toal:%ld\n", bytes_sent, total);
  }

  // printf("end seq\n");
  //  Ensure EOF packet is properly sent and handled
  //  packet eof_pkt = create_packet(END_OF_FILE, 0, total_size, "eof", 3);
  //  if (send_message(sockfd, eof_pkt) != 0) {
  //   perror("Send failed during EOF transfer");
  //   close(file_fd);
  //   return -1;
  // }

  // printf("Ack sended\n");
  //  Wait for acknowledgment from the server
  packet ack_pkt;
  if (rcv_message(sockfd, OK, END_OF_FILE, &ack_pkt) != 0) {
    perror("Failed to receive acknowledgment for EOF");
    close(file_fd);
    return -1;
  }

  //  printf("File sent successfully: %s (total size: %u bytes)\n", file_name,
  //       total_size);

  close(file_fd);
  return 0;
}

///
////
char *receive_file(int socket_fd, uint32_t *out_total_size,
                   FileInfo *file_info) {

  uint32_t total_size = 0;
  uint32_t received_size = 0;

  // Receive Metadata
  packet metadata_pkt;
  if (rcv_message(socket_fd, METADATA, 0, &metadata_pkt) != 0) {
    printf("Error receiving file metadata or connection closed.\n");
    return NULL;
  }
  // printf("Metadata Received\n");
  memcpy(file_info, metadata_pkt._payload, sizeof(FileInfo));

  // printf("Receiving file '%s' (last modified: %ld)\n", file_info->filename,
  //      file_info->last_modified);

  // Prepare to receive file data

  total_size = metadata_pkt.total_size;
  *out_total_size = total_size;

  char *file_data = malloc(total_size);
  if (!file_data) {
    printf("Memory allocation failed for %u bytes.\n", total_size);
    return NULL;
  }
  memset(file_data, 0, total_size);

  // printf("total size: %d, received size: %d\n", total_size, received_size);
  char buffer[MAX_PAYLOAD_SIZE];
  while (received_size < total_size) {
    // Receive raw bytes directly into the buffer
    ssize_t bytes_received = recv(socket_fd, buffer, MAX_PAYLOAD_SIZE, 0);

    if (bytes_received < 0) {
      perror("Error receiving data");
      free(file_data);
      return NULL;
    }

    if (bytes_received == 0) {
      // Connection closed by the sender
      printf("Connection closed by sender\n");
      break;
    }

    // Store the received bytes into the file data buffer
    memcpy(file_data + received_size, buffer, bytes_received);
    received_size += bytes_received;

    // printf("Received %ld bytes, total received = %u bytes\n", bytes_received,
    //     received_size);
  }

  // Receive the EOF packet
  // packet eof_pkt;
  // if (rcv_message(socket_fd, END_OF_FILE, 0, &eof_pkt) != 0) {
  //  printf("Error receiving EOF.\n");
  //  printf("seqn: %d, type: %d", eof_pkt.type, eof_pkt.seqn);
  //  free(file_data);
  //  return NULL;
  //}

  // Send acknowledgment for EOF
  packet ack_pkt = create_packet(OK, END_OF_FILE, total_size, "ack", 3);
  if (send_message(socket_fd, ack_pkt) != 0) {
    printf("Error sending acknowledgment for EOF.\n");
    free(file_data);
    return NULL;
  }

  // printf("File received successfully: '%s', total size = %u bytes\n",
  //       file_info->filename, total_size);

  return file_data;
}

int send_file_list(int sockfd, const char dir[MAX_FILENAME_SIZE]) {
  int file_count = 0;
  FileInfo *files = list_files(dir, &file_count);

  packet num_pkt = create_packet(OK, C_LIST_SERVER, file_count, "ok", 2);

  if (send_message(sockfd, num_pkt) != 0) {
    perror("Filed send num_pkt\n");
    return -1;
  }

  uint16_t seqn = 1;
  for (int i = 0; i < file_count; i++) {
    packet pkt = create_packet(C_LIST_SERVER, seqn, file_count, "ok", 2);

    memcpy(pkt._payload, &files[i], sizeof(FileInfo));
    pkt.length = sizeof(FileInfo);

    if (send_message(sockfd, pkt) != 0) {
      perror("Failed send pkt\n");
      return -1;
    }

    seqn++;
  }

  // Ensure EOF packet is properly sent and handled
  packet eof_pkt = create_packet(END_OF_FILE, seqn, file_count, "eof", 3);
  if (send_message(sockfd, eof_pkt) != 0) {
    perror("Send failed during EOF transfer");
    free(files);
    return -1;
  }

  // Wait for acknowledgment from the server
  packet ack_pkt;
  if (rcv_message(sockfd, OK, END_OF_FILE, &ack_pkt) != 0) {
    perror("Failed to receive acknowledgment for EOF");
    free(files);
    return -1;
  }

  free(files);
  return 0;
}

FileInfo *receive_file_list(int socket, int *file_count) {
  if (!file_count) {
    fprintf(stderr, "Invalid file_count pointer\n");
    return NULL;
  }

  // Receive the number of files
  packet num_pkt;
  if (rcv_message(socket, OK, C_LIST_SERVER, &num_pkt) != 0) {
    perror("Failed to receive number of files");
    return NULL;
  }

  *file_count = num_pkt.total_size;
  if (*file_count < 0) {
    fprintf(stderr, "Invalid file count received: %d\n", *file_count);
    return NULL;
  }

  FileInfo *files = malloc(sizeof(FileInfo) * (*file_count));
  if (!files) {
    perror("Failed to allocate memory for file list");
    return NULL;
  }

  uint32_t received_files = 0;
  uint16_t seqn = 1;
  while (received_files < *file_count) {
    packet file_pkt;
    if (rcv_message(socket, C_LIST_SERVER, seqn, &file_pkt) != 0) {
      perror("Failed to receive file packet");
      free(files);
      return NULL;
    }

    if (file_pkt.length != sizeof(FileInfo)) {
      fprintf(stderr, "Unexpected packet length: %u\n", file_pkt.length);
      free(files);
      return NULL;
    }

    memcpy(&files[received_files], file_pkt._payload, sizeof(FileInfo));
    received_files++;
    seqn++;
  }

  // Receive the EOF packet
  packet eof_pkt;
  if (rcv_message(socket, END_OF_FILE, seqn, &eof_pkt) != 0) {
    perror("Failed to receive EOF packet");
    free(files);
    return NULL;
  }

  // Send acknowledgment for EOF
  packet ack_pkt = create_packet(OK, END_OF_FILE, *file_count, "ack", 3);
  if (send_message(socket, ack_pkt) != 0) {
    perror("Failed to send acknowledgment for EOF");
    free(files);
    return NULL;
  }

  return files;
}

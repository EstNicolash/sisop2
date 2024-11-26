#include "../headers/protocol.h"

packet create_packet(uint16_t type, uint16_t seqn, uint32_t total_size,
                     const char *_payload) {
  packet pkt;
  pkt.type = type;
  pkt.seqn = seqn;
  pkt.total_size = total_size;

  memset(pkt._payload, 0, MAX_PAYLOAD_SIZE);
  size_t payload_length = strnlen(_payload, MAX_PAYLOAD_SIZE);
  memcpy(pkt._payload, _payload, payload_length);

  pkt.length = (uint16_t)payload_length;

  return pkt;
}

int send_message(int sockfd, packet pkt) {
  ssize_t bytes_sent = write(sockfd, &pkt, sizeof(pkt));
  if (bytes_sent != sizeof(pkt)) {
    perror("Failed to send packet");
    return -1;
  }
  return 0;
}

int rcv_message(int sockfd, uint16_t type, uint16_t seqn, packet *rcv_pkt) {
  ssize_t bytes_received = read(sockfd, rcv_pkt, sizeof(*rcv_pkt));
  if (bytes_received != sizeof(*rcv_pkt)) {
    perror("Failed to receive packet");
    return -1;
  }

  // Validate the packet's type and sequence number
  if (rcv_pkt->type != type || rcv_pkt->seqn != seqn) {
    fprintf(stderr, "Packet verification failed (type or seqn mismatch)\n");
    return -1;
  }

  return 0;
}

int send_file(int sockfd, const char file_name[MAX_FILENAME_SIZE]) {

  size_t file_size = 0;
  FileInfo metadata = get_file_info(file_name);
  char *file_buffer = load_file(file_name, &file_size);

  if (file_buffer == NULL) {
    fprintf(stderr, "Failed to load file: %s\n", file_name);
    return -1;
  }

  // Send file metadata first
  //
  packet metadata_pkt = create_packet(METADATA, 0, 0, (char *)&metadata);
  if (send_message(sockfd, metadata_pkt) < 0) {
    fprintf(stderr, "Failed to send metadata\n");
    free(file_buffer);
    return -1;
  }
  send_message(sockfd, metadata_pkt);

  // Wait for acknowledgment
  packet ack_pkt;
  if (rcv_message(sockfd, OK, 0, &ack_pkt) < 0) {
    fprintf(stderr, "Failed to receive acknowledgment for metadata\n");
    free(file_buffer);
    return -1;
  }

  // TOtal fragment to be send
  uint32_t total_fragments =
      (file_size + MAX_PAYLOAD_SIZE - 1) / MAX_PAYLOAD_SIZE;

  // Send the file_buffer
  uint32_t seq = 1;

  while (seq <= total_fragments) {
    size_t offset = (seq - 1) * MAX_PAYLOAD_SIZE;
    size_t chunk_size = (seq == total_fragments)
                            ? (file_size % MAX_PAYLOAD_SIZE)
                            : MAX_PAYLOAD_SIZE;

    packet data_pkt = create_packet(SEND, (uint16_t)seq, total_fragments,
                                    &file_buffer[offset]);

    if (send_message(sockfd, data_pkt) != 0) {
      fprintf(stderr, "Failed to send fragment %u\n", seq);
      free(file_buffer);
      return -1;
    }

    seq++;
  }

  free(file_buffer);

  // Send EOF packet
  packet eof_pkt = create_packet(EOF, seq, 0, "eof");
  if (send_message(sockfd, eof_pkt) < 0) {
    fprintf(stderr, "Failed to send EOF packet\n");
    return -1;
  }

  // Wait for final acknowledgment
  if (rcv_message(sockfd, OK, 0, &ack_pkt) < 0) {
    fprintf(stderr, "Failed to receive acknowledgment for EOF\n");
    return -1;
  }

  return 0;
}
char *receive_file(int sockfd, FileInfo *fileinfo) {
  // Receive file metadata
  packet metadata_pkt;
  if (rcv_message(sockfd, METADATA, 0, &metadata_pkt) < 0) {
    fprintf(stderr, "Failed to receive file metadata\n");
    return NULL;
  }

  memcpy(fileinfo, metadata_pkt._payload, sizeof(FileInfo));

  char *file_buffer = malloc(fileinfo->file_size);
  if (file_buffer == NULL) {
    fprintf(stderr, "Failed to allocate memory for file buffer\n");
    return NULL;
  }

  uint32_t total_fragments = metadata_pkt.total_size;
  uint32_t seq = 1;
  while (seq <= total_fragments) {
    packet data_pkt;
    if (rcv_message(sockfd, SEND, seq, &data_pkt) != 0) {
      fprintf(stderr, "Failed to receive fragment %u\n", seq);
      free(file_buffer);
      return NULL;
    }

    size_t offset = (seq - 1) * MAX_PAYLOAD_SIZE;
    size_t chunk_size = data_pkt.length;
    memcpy(&file_buffer[offset], data_pkt._payload, chunk_size);

    seq++;
  }

  packet eof_pkt;
  if (rcv_message(sockfd, EOF, seq, &eof_pkt) < 0) {
    fprintf(stderr, "Failed to receive EOF packet\n");
    free(file_buffer);
    return NULL;
  }

  packet ack = create_packet(OK, 0, 0, "ack");
  send_message(sockfd, ack);

  return file_buffer;
}

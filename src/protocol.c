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

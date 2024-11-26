#include "../headers/client_commands.h"
#include "../headers/protocol.h"

int client_send_id(int sockfd, char client_id[MAX_FILENAME_SIZE]) {

  packet pkt = create_packet(C_SEND_ID, 0, 0, client_id);

  if (send_message(sockfd, pkt) != 0)
    return -1;

  if (rcv_message(sockfd, OK, 0, &pkt) != 0)
    return -1;

  return 0;
}

#include "../headers/client_commands.h"
#include "../headers/protocol.h"

int client_send_id(int sockfd, char client_id[MAX_FILENAME_SIZE]) {

  packet pkt = create_packet(C_SEND_ID, 0, 0, client_id, strlen(client_id));

  if (send_message(sockfd, pkt) != 0)
    return -1;

  if (rcv_message(sockfd, OK, 0, &pkt) != 0)
    return -1;

  return 0;
}

int client_upload_file(int sockfd, char filename[MAX_FILENAME_SIZE]) {

  packet upload_msg = create_packet(C_UPLOAD, 0, 0, "upload", 6);

  if (send_message(sockfd, upload_msg) != 0) {
    fprintf(stderr, "Error send upload msg\n");
    return -1;
  };

  if (rcv_message(sockfd, OK, C_UPLOAD, &upload_msg) != 0) {
    fprintf(stderr, "Error rcv ack in upload\n");
    return -1;
  };
  printf("send\n");
  return send_file(sockfd, filename);
}

int client_download_file(int sockfd, char filename[MAX_FILENAME_SIZE]) {
  return 0;
}

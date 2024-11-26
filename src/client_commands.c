#include "../headers/client_commands.h"
#include "../headers/protocol.h"
#include <stdint.h>

int client_send_id(int sockfd, char client_id[MAX_FILENAME_SIZE]) {

  packet pkt = create_packet(C_SEND_ID, 0, 0, client_id, strlen(client_id));

  if (send_message(sockfd, pkt) != 0)
    return -1;

  if (rcv_message(sockfd, OK, 0, &pkt) != 0)
    return -1;

  return 0;
}

int client_upload_file(int sockfd, char filename[MAX_FILENAME_SIZE]) {

  if (file_exists(filename) != 0) {
    fprintf(stderr, "File doesn't exist\n");
    return -1;
  }

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

int client_download_file(int sockfd, char filename[MAX_FILENAME_SIZE],
                         char *dir) {

  packet download_msg = create_packet(C_DOWNLOAD, 0, 0, "download", 8);

  if (send_message(sockfd, download_msg) != 0) {
    fprintf(stderr, "Error send download msg\n");
    return -1;
  };

  download_msg = create_packet(C_DOWNLOAD, 1, 0, filename, strlen(filename));

  if (send_message(sockfd, download_msg) != 0) {
    fprintf(stderr, "Error send download file name\n");
    return -1;
  };

  printf("waiting ack\n");
  // ACK
  if (rcv_message(sockfd, OK, C_DOWNLOAD, &download_msg) != 0) {
    fprintf(stderr, "Error rcv ack download or file doesn't exist \n");
    return -1;
  };

  uint32_t out_total_size = 0;
  FileInfo fileinfo;

  char *file_data = receive_file(sockfd, &out_total_size, &fileinfo);

  save_file(fileinfo.filename, dir, file_data, out_total_size);

  free(file_data);

  return 0;
}

int client_list_client(int sockfd) {

  int file_count = 0;
  const char path[MAX_FILENAME_SIZE] = SYNC_DIR;
  FileInfo *files = list_files(path, &file_count);
  print_file_list(files, file_count);

  return 0;
}

int client_list_server(int sockfd) { return 0; }

int client_delete_file(int sockfd, char filename[MAX_FILENAME_SIZE]) {
  return 0;
}

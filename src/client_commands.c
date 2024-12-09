#include "../headers/client_commands.h"
#include "../headers/protocol.h"
#include <stdint.h>
#include <string.h>
#include <openssl/evp.h>
int client_exit(int sockfd) {
  packet pkt = create_packet(C_EXIT, 0, 0, "exit", 4);
  send_message(sockfd, pkt);
  return 0;
}
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
  // printf("send\n");
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

  // printf("waiting ack\n");
  //  ACK
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
  free(files);

  return 0;
}

int client_list_server(int sockfd) {
  packet msg_pkt = create_packet(C_LIST_SERVER, 0, 0, "ok", 2);

  if (send_message(sockfd, msg_pkt)) {
    perror("Fail sending list_server\n");
    return -1;
  }

  int file_count = 0;
  FileInfo *files = receive_file_list(sockfd, &file_count);

  if (files == NULL) {
    perror("error receiving files list from server\n");
    return -1;
  }

  print_file_list(files, file_count);
  free(files);

  return 0;
}

int client_delete_file(int sockfd, char filename[MAX_FILENAME_SIZE]) {

  packet delete_msg = create_packet(C_DELETE, 0, 0, "delete", 6);

 // printf("delete msg\n");
  if (send_message(sockfd, delete_msg) != 0) {
    perror("Failed to send delete_msg\n");
    return -1;
  }

  strncpy(delete_msg._payload, filename, MAX_FILENAME_SIZE);
  delete_msg.length = strlen(delete_msg._payload);

  //printf("file name\n");
  if (send_message(sockfd, delete_msg) != 0) {
    perror("Failed to send delete_msg\n");
    return -1;
  }

  if (rcv_message(sockfd, OK, C_DELETE, &delete_msg) != 0) {
    perror("error in deleting or file doesn't exist\n");
    return -1;
  }

  printf("delete acked\n");
  return 0;
}

int get_sync_dir(int sockfd) {

  packet get_pkt = create_packet(C_GET_SYNC_DIR, 0, 0, "get", 3);

  if (send_message(sockfd, get_pkt) != 0) {
    perror("Error sending get_pkt\n");
    return -1;
  }

  if (rcv_message(sockfd, OK, C_GET_SYNC_DIR, &get_pkt) != 0) {
    perror("wrong get ACK\n");
    return -1;
  }

  // printf("Sync Dir acked\n");

  // Local file LISt
  int local_file_count = 0;
  const char path[MAX_FILENAME_SIZE] = SYNC_DIR;
  FileInfo *local_files = list_files(path, &local_file_count);

  // Get server file list
  packet msg_pkt = create_packet(C_LIST_SERVER, 0, 0, "ok", 2);

  if (send_message(sockfd, msg_pkt) != 0) {
    perror("Fail sending list_server\n");
    return -1;
  }

  int server_file_count = 0;
  FileInfo *server_files = receive_file_list(sockfd, &server_file_count);

  FileInfo *to_delete_files = malloc(local_file_count * sizeof(FileInfo));
  FileInfo *to_download_files = malloc(server_file_count * sizeof(FileInfo));
  if (!to_delete_files || !to_download_files) {
    perror("Memory allocation failed\n");
    return -1;
  }

  int to_delete_count = 0;
  int to_download_count = 0;

  // Identify files to download
  for (int i = 0; i < server_file_count; i++) {
    int found = 0;
    for (int j = 0; j < local_file_count; j++) {
      if (strcmp(server_files[i].filename, local_files[j].filename) == 0) {
        found = 1;

        if ((server_files[i].last_modified > local_files[j].last_modified) && 
            (!memcmp(server_files[i].md5_checksum, local_files[j].md5_checksum, 16))){
          to_download_files[to_download_count++] = server_files[i];
        }
        break;
      }
    }
    if (!found) {
      to_download_files[to_download_count++] = server_files[i];
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
    snprintf(file_path, sizeof(file_path), "%s/%s", SYNC_DIR,
             to_delete_files[i].filename);
    delete_file(file_path);
  }

  for (int i = 0; i < to_download_count; i++) {
    client_download_file(sockfd, to_download_files[i].filename, SYNC_DIR);
  }

  free(to_delete_files);
  free(to_download_files);

  return 0;
}

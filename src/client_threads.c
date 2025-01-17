#include "../headers/client_threads.h"
#include <sys/inotify.h>

int is_sync_running = 0;
int is_inotify_running = 0;
int is_messages_running = 0;
int is_rcv_propagation_running = 0;

IgnoreEntry ignore_files[MAX_IGNORE_FILES];
TimedIgnoreEntry timed_ignore_files[MAX_IGNORE_FILES];
pthread_mutex_t ignore_mutex = PTHREAD_MUTEX_INITIALIZER;

void *rcv_propagation_thread(void *arg) {

  packet pkt;

  while (is_rcv_propagation_running == 0) {
    char msg[MAX_PAYLOAD_SIZE];
    fprintf(stderr, "rcv_propagation_thread: receiveing rcv_message\n");
    if (rcv_message(prop_read_sockfd, S_PROPAGATE, 0, &pkt) == -1)
      continue;

    //  strncpy(msg, pkt._payload, MAX_PAYLOAD_SIZE);
    snprintf(msg, MAX_PAYLOAD_SIZE, "%s", pkt._payload);
    fprintf(stderr, "rcv_propagation_thread msg: %s", msg);
    msg_queue_insert_start(S_PROPAGATE, msg);
  }

  return NULL;
}

void *messages_thread(void *arg) {

  while (is_messages_running == 0) {

    struct message_queue *msg = msg_queue_remove();

    if (msg == NULL)
      continue;

    fprintf(stderr, "msg_type: %d", msg->msg_type);
    switch (msg->msg_type) {

    case C_DOWNLOAD: {

      char cwd[MAX_FILENAME_SIZE];

      if (getcwd(cwd, sizeof(cwd)) != NULL) {
        printf("Current working directory: %s\n", cwd);
      } else {
        perror("getcwd() error");
      }
      client_download_file(sockfd, msg->msg_info, cwd);
      break;
    }
    case C_DELETE:
      client_delete_file(sockfd, msg->msg_info);
      break;
    case C_UPLOAD:
      client_upload_file(sockfd, msg->msg_info);
      break;

    case C_LIST_SERVER:
      client_list_server(sockfd);
      break;

    case C_GET_SYNC_DIR: {
      if (get_sync_dir(sockfd) != 0) {
        fprintf(stderr, "Error syncing directories\n");
      }
      break;
    }

    case S_PROPAGATE: {

      fprintf(stderr, "info: %s\n", msg->msg_info);
      if (strcmp("upload", msg->msg_info) == 0) {

        fprintf(stderr, "up\n");
        client_rcv_propagation(prop_write_sockfd);
      } else if (strcmp("delete", msg->msg_info) == 0) {

        fprintf(stderr, "del\n");
        client_delete_propagation(prop_write_sockfd);
      }
      break;
    }

    default:
      fprintf(stderr, "Sus type in queue: %d\n", msg->msg_type);
      break;
    }

    free(msg);
  }

  return NULL;
}
void monitor_sync_dir() {

  int inotifyFd = inotify_init();
  if (inotifyFd == -1) {
    perror("Error initializing inotify");
    return;
  }

  int wd = inotify_add_watch(inotifyFd, SYNC_DIR,
                             IN_CREATE | IN_DELETE | IN_MODIFY | IN_MOVED_FROM);

  if (wd == -1) {
    perror("Error adding watch");
    close(inotifyFd);
    return;
  }

  char buf[4096];
  ssize_t numRead;

  while (is_inotify_running == 0) {
    numRead = read(inotifyFd, buf, sizeof(buf));
    if (numRead == 0)
      break;
    if (numRead == -1) {
      perror("Error reading events");
      break;
    }

    for (char *p = buf; p < buf + numRead;) {
      struct inotify_event *event = (struct inotify_event *)p;
      char file_path[MAX_PAYLOAD_SIZE * 2];
      snprintf(file_path, sizeof(file_path), "%s/%s", SYNC_DIR, event->name);
      fprintf(stderr, "\t monitor: %s\n", file_path);
      if (is_inotify_running != 0)
        break;

      if (event->name[0] == '.')
        break;

      if (event->mask & IN_MODIFY || event->mask & IN_CREATE) {

        fprintf(stderr, "\t monitor[MODIFY or CREATE]: %s\n", event->name);
        fprintf(stderr, "\t is_timed_ignored and is_ignored: [%d,%d]\n",
                is_timed_ignored(file_path), is_ignored(file_path));
        if (is_timed_ignored(file_path) == 0 && is_ignored(file_path) == 0) {
          msg_queue_insert(C_UPLOAD, file_path);
          add_to_timed_ignore_list(file_path); // Prevent re-trigger
        }
      }

      if (event->mask & IN_DELETE || event->mask & IN_MOVED_FROM) {

        char file_path[MAX_PAYLOAD_SIZE * 2];
        snprintf(file_path, sizeof(file_path), "%s/%s", SYNC_DIR, event->name);
        delete_file(file_path);
        msg_queue_insert(C_DELETE, event->name);
      }

      p += sizeof(struct inotify_event) + event->len;
    }
  }
  close(inotifyFd);
}
void *inotify_thread(void *arg) {
  monitor_sync_dir();
  return NULL;
}

// Thread function to periodically sync directories
void *sync_dir_thread(void *arg) {

  char msg[MAX_PAYLOAD_SIZE] = {""};
  while (is_sync_running == 0) {

    msg_queue_insert(C_GET_SYNC_DIR, msg);
    /*
    pthread_mutex_lock(&client_sync_mutex);
    // printf("Starting sync...\n");
    if (get_sync_dir(sockfd) != 0) {
      fprintf(stderr, "Error syncing directories\n");
    }
    pthread_mutex_unlock(&client_sync_mutex);*/
    sleep(300);
  }
  printf("Sync thread exiting...\n");
  return NULL;
}

void *heartbeat_thread(void *arg) {

  while (1) {
    int temp_sockfd;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(HEARTBEAT_PORT);

    inet_pton(AF_INET, server_ip, &addr.sin_addr);

    temp_sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (temp_sockfd < 0) {
      perror("Socket creation failed");
      sleep(1);
      continue;
    }

    sleep(HEARTBEAT_INTERVAL);

    // fprintf(stderr, "Sending heartbet\n");
    if (connect(temp_sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {

      sleep(HEARTBEAT_INTERVAL);

      close(sockfd);
      close(prop_read_sockfd);
      close(prop_write_sockfd);

      fprintf(stderr, "Next server = %s\n", next_server_ip);
      sockfd = client_connect(next_server_ip, port);
      sleep(1);
      prop_read_sockfd = client_connect(next_server_ip, port + 1);
      sleep(1);
      prop_write_sockfd = client_connect(next_server_ip, port + 2);

      fprintf(stderr, "Sockets = %d,%d,%d\n", sockfd, prop_read_sockfd,
              prop_write_sockfd);

      strcpy(server_ip, next_server_ip);

      client_send_id(sockfd, client_id);

      fprintf(stderr, "Next server = %s\n", next_server_ip);
    }

    continue;

    close(temp_sockfd); // Close socket after successful heartbeat
  }
}

int client_connect(const char *server_ip, int port) {
  int temp_sockfd;
  struct sockaddr_in serv_addr;

  if ((temp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("ERROR opening socket");
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  if (inet_pton(AF_INET, server_ip, &serv_addr.sin_addr) <= 0) {
    perror("Invalid IP address or address not supported");
    close(temp_sockfd);
    return -1;
  }

  printf("Attempting connection\n");
  if (connect(temp_sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) <
      0) {
    perror("ERROR connecting");
    close(temp_sockfd);
    return -1;
  }

  printf("Connected\n");
  return temp_sockfd;
}

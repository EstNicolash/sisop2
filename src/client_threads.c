#include "../headers/client_threads.h"
#include <sys/inotify.h>

int is_sync_running = 0;
int is_inotify_running = 0;
int is_messages_running = 0;
int is_rcv_propagation_running = 0;

IgnoreList ignore_files[50];
pthread_mutex_t ignore_mutex = PTHREAD_MUTEX_INITIALIZER;

void *rcv_propagation_thread(void *arg) {

  int sockfd = *(int *)arg;

  packet pkt;

  while (is_rcv_propagation_running == 0) {
    char msg[MAX_PAYLOAD_SIZE];
    rcv_message(sockfd, S_PROPAGATE, 0, &pkt);
    strncpy(msg, pkt._payload, MAX_PAYLOAD_SIZE);
    fprintf(stderr, "msg: %s", msg);
    msg_queue_insert_start(S_PROPAGATE, msg);
  }

  return NULL;
}

void *messages_thread(void *arg) {

  int *sockets = (int *)arg;
  int sockfd = sockets[0];
  int prop_sockfd = sockets[1];
  free(arg);

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
        client_rcv_propagation(prop_sockfd);
      } else if (strcmp("delete", msg->msg_info) == 0) {

        fprintf(stderr, "del\n");
        client_delete_propagation(prop_sockfd);
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
void monitor_sync_dir(int sockfd) {

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

      if (is_inotify_running != 0)
        break;

      if (event->name[0] == '.')
        break;

      if (event->mask & IN_MODIFY || event->mask & IN_CREATE) {

        fprintf(stderr, "MODIFY or CREATE: %s", event->name);
        if (!is_ignored(file_path)) {
          msg_queue_insert(C_UPLOAD, file_path);
          add_to_ignore_list(file_path); // Prevent re-trigger
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
  int sockfd = *(int *)arg;
  monitor_sync_dir(sockfd);
  return NULL;
}

// Thread function to periodically sync directories
void *sync_dir_thread(void *arg) {
  // int sockfd = *(int *)arg;

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

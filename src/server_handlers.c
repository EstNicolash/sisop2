#include "../headers/server_handlers.h"
#include <string.h>

Client manage_clients_connections[MAX_CLIENTS] = {0};
int client_count = 0;
pthread_mutex_t manage_clients_mutex = PTHREAD_MUTEX_INITIALIZER;

int server_handles_id(int sockfd, char user_id[MAX_FILENAME_SIZE]) {

  packet rcv_pkt;

  if (rcv_message(sockfd, C_SEND_ID, 0, &rcv_pkt) != 0)
    return -1;

  strncpy(user_id, rcv_pkt._payload, MAX_FILENAME_SIZE);

  packet pkt = create_packet(OK, 0, 0, "ok");
  send_message(sockfd, pkt);
  return 1;
}

int update_connection_count(const char *user_id, int delta) {
  pthread_mutex_lock(&manage_clients_mutex);

  for (int i = 0; i < client_count; i++) {
    if (strcmp(manage_clients_connections[i].user_id, user_id) == 0) {
      // Check if adding a connection exceeds the limit
      if (delta > 0 && manage_clients_connections[i].connection_count >=
                           MAX_CONNECTIONS_PER_CLIENT) {
        pthread_mutex_unlock(&manage_clients_mutex);
        return -1;
      }

      // Update the connection count
      manage_clients_connections[i].connection_count += delta;

      // If the connection count drops to zero, remove the client
      if (manage_clients_connections[i].connection_count <= 0) {
        manage_clients_connections[i] =
            manage_clients_connections[--client_count];
      }

      pthread_mutex_unlock(&manage_clients_mutex);
      return 0;
    }
  }

  // If the client is not found and we are adding a new connection
  if (delta > 0 && client_count < MAX_CLIENTS) {
    strcpy(manage_clients_connections[client_count].user_id, user_id);
    manage_clients_connections[client_count].connection_count = delta;
    client_count++;
    pthread_mutex_unlock(&manage_clients_mutex);
    return 0;
  }

  pthread_mutex_unlock(&manage_clients_mutex);
  return -1;
}

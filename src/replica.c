#include "../headers/replica.h"
#include "../headers/protocol.h"
#include "../headers/file_manager.h"
int replica_sockets[3] = {-1};
int setup_replica_socket() {
  struct sockaddr_in addr;
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);

  if (sockfd < 0) {
    perror("Failed to create socket");
    exit(EXIT_FAILURE);
  }

  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(REPLICA_PORT);

  if (bind(sockfd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("Bind failed");
    exit(EXIT_FAILURE);
  }

  if (listen(sockfd, 5) < 0) {
    perror("Listen failed");
    exit(EXIT_FAILURE);
  }

  fprintf(stderr, "setup replica sockfd\n");
  return sockfd;
}

void *listen_for_replica_connection(void *arg){
  int *sock = (int*)arg;
  int server_fd = *sock;

  while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);
    int *client_fd = malloc(sizeof(int));
    fprintf(stderr, "listen replica\n");	  
    *client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_len);

    if (*client_fd < 0) {
      perror("Accept failed");
      free(client_fd);
      continue;
    }


    pthread_t tid;
    if (pthread_create(&tid, NULL, replica_handler, client_fd) != 0) {
      perror("Thread creation failed");
      close(*client_fd);
      free(client_fd);
    }

    pthread_detach(tid);
  }
  
  return NULL;
}

void *replica_handler(void *arg) {
  int *sock = (int*)arg;
  int sockfd = *sock;

  printf("Client replica_handler\n");
	
  while (1) {

  char user_id[MAX_FILENAME_SIZE] = {0};
    packet received_packet;
    if (rcv_message(sockfd, ANYTHING, 0, &received_packet) != 0) {
      fprintf(stderr, "Failed to receive packet");
    };


    fprintf(stderr, "msg readed\n");
    if(received_packet.type == C_UPLOAD){
      strncpy(user_id, received_packet._payload, MAX_FILENAME_SIZE);

     create_directory(user_id);
     
     packet ack_pkt = create_packet(OK,S_PROPAGATE,0,"ok",2);
     fprintf(stderr, "sendig first ack replica\n");
     send_message(sockfd, ack_pkt);

     FileInfo server_file_metada = rcv_metadata(sockfd);

     if (server_file_metada.file_size == 0) {

          packet ack_err = create_packet(ERROR, ERROR, 0, "no", 2);
          send_message(sockfd, ack_err);
          fprintf(stderr, "sendig first error replica\n");
          continue;
     }

     fprintf(stderr, "sendig ack replica 2\n");
     send_message(sockfd, ack_pkt);
     uint32_t out_total_size;
     FileInfo fileinfo;	
      fprintf(stderr, "receiving file\n");	    
     char *file_data = receive_file(sockfd, &out_total_size, &fileinfo);
     save_file(fileinfo.filename, user_id, file_data, out_total_size);
    
    	continue;
    }
    
    
    if(received_packet.type == C_DELETE){
    
    
	  packet ack = create_packet(OK, C_DELETE, 0, "ok", 2);

	  fprintf(stderr, "Teste A\n");
	  // Send ack
	  
	  if (send_message(sockfd, ack) != 0) {
	    perror("Error sending ack msg\n");
	    continue;
	  }

	  fprintf(stderr, "Teste B\n");
	  // rcv metada

	  packet msg;
	  if (rcv_message(sockfd, S_PROPAGATE, C_DELETE, &msg) != 0) {
	    perror("Error rcv_message ack propagate\n");
	    continue;
	  }

	  char file_path[MAX_PAYLOAD_SIZE * 2];

	
	  snprintf(file_path, sizeof(file_path), "%s/%s", user_id, msg._payload);
	  delete_file(file_path);
	  send_message(sockfd, ack);
	  
    	continue;
    }
    
   
	    


  }
	

  printf("Client replica_handler exit\n");

  return NULL;
}


int propagate_to_backup(int sockfd, const char user_id[MAX_FILENAME_SIZE],
                        const char filename[MAX_FILENAME_SIZE]) {

  packet msg = create_packet(C_UPLOAD, 0, 0, "upload", 6);
  strncpy(msg._payload, user_id, MAX_FILENAME_SIZE);
  msg.length = strlen(msg._payload);


  if (send_message(sockfd, msg) != 0) {
    perror("Error sending propagate msg\n");
    return -1;
  }


  fprintf(stderr, "Receiving ack replica\n");
  // rcv Ack client command
  if (rcv_message(sockfd, OK, C_UPLOAD, &msg) != 0) {
    perror("Error rcv_message ack propagate\n");
    return -1;
  }

  fprintf(stderr, "Sending metadata\n");
  char file_path[MAX_PAYLOAD_SIZE * 2];
  snprintf(file_path, sizeof(file_path), "%s/%s", user_id, filename);

  if (send_metadata(sockfd, file_path) != 0) {
    fprintf(stderr, "error or equal checksum\n");
    return -1;
  }

  fprintf(stderr, "Receiving ACK\n");
  if (rcv_message(sockfd, OK, C_UPLOAD, &msg) != 0) {
    perror("Error rcv_message ack propagate or checksum or no file\n");
    return -1;
  }

  fprintf(stderr, "Teste 5\n");
  return send_file(sockfd, file_path);
}

int propagate_delete_to_backup(int sockfd, const char user_id[MAX_FILENAME_SIZE],
                     const char filename[MAX_FILENAME_SIZE]) {

  packet msg = create_packet(C_DELETE, 0, 0, "a", 1);
  strncpy(msg._payload, user_id, MAX_FILENAME_SIZE);
  msg.length = strlen(msg._payload);
 
  if (send_message(sockfd, msg) != 0) {
    perror("Error sending propagate msg\n");
    return -1;
  }
  
  fprintf(stderr, "Teste 1\n");
  
  if (rcv_message(sockfd, OK, C_DELETE, &msg) != 0) {
    perror("Error rcv_message ack propagate\n");
    return -1;
  }

  fprintf(stderr, "Teste 2\n");

  packet to_delete =
      create_packet(S_PROPAGATE, C_DELETE, 0, filename, strlen(filename));
  if (send_message(sockfd, to_delete) != 0) {
    fprintf(stderr, "error or equal checksum\n");
    return -1;
  }

  fprintf(stderr, "Teste 4\n");
  
  if (rcv_message(sockfd, OK, C_DELETE, &msg) != 0) {
    perror("Error rcv_message ack propagate or checksum or no file\n");
    return -1;
  }

  fprintf(stderr, "Teste 5\n");
  return 0;
}

int replica_connect(const char *server_ip) {
  int temp_sockfd;
  struct sockaddr_in serv_addr;

  if ((temp_sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    perror("ERROR opening socket");
    return -1;
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(REPLICA_PORT);

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

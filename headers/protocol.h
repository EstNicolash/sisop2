#ifndef PROTOCOL_H
#define PROTOCOL_H
#include "file_manager.h"

#include <stdint.h>
#define MAX_PAYLOAD_SIZE 4096

#define ERROR 0
#define OK 1

#define SEND 10
#define RECEIVE 11
#define END_OF_FILE 13

#define C_SEND_ID 20
#define C_UPLOAD 21
#define C_DOWNLOAD 22
#define C_DELETE 23
#define C_LIST_SERVER 24
#define C_GET_SYNC_DIR 29

// Estrutura para troca de mensagens de arquivo
typedef struct packet {
  uint16_t type;                   // Tipo do pacote (ex: DATA, CMD)
  uint16_t seqn;                   // Número de sequência
  uint32_t total_size;             // Número total de fragmentos
  uint16_t length;                 // Comprimento do payload
  char _payload[MAX_PAYLOAD_SIZE]; // Dados do pacote
} packet;

packet create_packet(uint16_t type, uint16_t seqn, uint32_t total_size,
                     uint16_t length, char _payload[MAX_PAYLOAD_SIZE]);

int send_message(packet pkt);

// Receive a packet and check the type and seqn with the intended received
// packet
int rcv_message(uint16_t type, uint16_t seqn, packet *rcv_pkt);

int send_file(int sockfd, const char filename[MAX_FILENAME_SIZE]);
char *receive_file(int sockfd, uint32_t *out_total_size, FileInfo *fileinfo);
int send_file_list(int sockfd, const char filename[MAX_FILENAME_SIZE]);
FileInfo *receive_file_list(int socket, int *file_count);

#endif

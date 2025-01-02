#ifndef REPLICA_H
#define REPLICA_H

extern int replica_m_sockfd;

int replica_rcv_propataion();
int primary_send_propagation();

#endif

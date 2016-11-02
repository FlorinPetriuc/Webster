#ifndef _SERVER_H_
#define _SERVER_H_

int start_server(const unsigned short int port);
int accept_conection(const int server_socket, unsigned char *ip, socklen_t *ip_len);

void close_connection(const int client_socket);

#endif
#ifndef _SOCKET_H_
#define _SOCKET_H_

#include "base.h"

sock *socket_init();
int socket_listen(server *srv);
int socket_accept(sock *s);
int socket_read(int fd, char *buffer, int len);
int socket_close(sock *s);
int socket_free(sock *s);
int socket_setup(master_server *master_srv, int fd);

#endif

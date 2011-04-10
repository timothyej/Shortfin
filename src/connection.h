#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <fcntl.h>
#include <netinet/tcp.h>
#include <time.h>
#include <sys/uio.h>
#include <sys/epoll.h>

#include <sys/sendfile.h>

#include "base.h"
#include "safe.h"

connection *connection_init();
connection *connection_setup(master_server *master_srv);
int connection_start(master_server *master_srv, connection *conn);
int connection_handle(connection *conn);
int connection_reset(master_server *master_srv, connection *conn);
int connection_free(master_server *master_srv, connection *conn);

#endif

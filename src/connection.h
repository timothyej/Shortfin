#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include "base.h"

connection **connection_setup(master_server *master_srv);
int connection_start(master_server *master_srv, connection *conn);
int connection_handle(worker *w, connection *conn);
int connection_reset(master_server *master_srv, connection *conn);
int connection_free(master_server *master_srv, connection *conn);

#endif

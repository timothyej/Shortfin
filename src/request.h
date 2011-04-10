#ifndef _REQUEST_H_
#define _REQUEST_H_

#include "base.h"

request *request_init(server *srv);
int request_parse(server *srv, connection *conn);
int request_free(request *req);

#endif

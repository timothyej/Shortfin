#ifndef _PROXY_H_
#define _PROXY_H_

#include "base.h"
#include "safe.h"

/* a simple reverse proxy */

proxy *proxy_init();
int proxy_match_request(server *srv, request *req, connection *conn);
int proxy_add_rule(proxy *p, char *value);
int proxy_free(proxy *p);

#endif

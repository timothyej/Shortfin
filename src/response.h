#ifndef _RESPONSE_H_
#define _RESPONSE_H_

#include "base.h"

response *response_init(server *srv);
int response_build(server *srv, connection *conn);

int response_build_http_packet(server *srv, response *resp);
int response_read_file(server *srv, connection *conn, file_item *f);
int response_file_exist(char *filename);
int response_get_index_file(server *srv, file_item *f);
int response_free(response *resp);

char *response_char2hex(char *str);

#endif

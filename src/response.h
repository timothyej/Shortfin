#ifndef _RESPONSE_H_
#define _RESPONSE_H_

#include "base.h"

int response_build(server *srv, connection *conn);
int response_free(response *resp);

#endif

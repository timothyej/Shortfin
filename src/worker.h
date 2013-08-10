#ifndef _WORKER_H_
#define _WORKER_H_

#include "base.h"

worker *worker_init(master_server *master_srv, int num);
int worker_spawn(int number, master_server *master_srv);
int worker_free(worker *w);

#endif

#ifndef _EVENTS_H_
#define _EVENTS_H_

#include "base.h"

event_handler *events_create(int max_clients);
int events_add_event(event_handler *evhand, int fd);
int events_add_event_2(event_handler *evhand, int fd);
int events_del_event(event_handler *evhand, int fd);
int events_wait(event_handler *evhand, master_server *master_srv);
int events_closed(event_handler *evhand, int num);
int events_get_fd(event_handler *evhand, int num);
int events_free(event_handler *evhand, int max_clients);

#endif

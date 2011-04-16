#ifndef _EVENTS_H_
#define _EVENTS_H_

#ifdef HAVE_SYS_EPOLL_H
	#include <sys/epoll.h>
#elif defined(HAVE_SYS_EVENT_H)
	#include <sys/types.h>
	#include <sys/event.h>
	#include <sys/time.h>
#endif

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

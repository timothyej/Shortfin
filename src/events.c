#include "events.h"

event_handler *events_create(int max_clients) {
	/* create a new event handler (epoll, kqueue etc.) */
	int i;
	event_handler *evhand = malloc(sizeof(event_handler));

	/* epoll */
	evhand->events = malloc(max_clients * sizeof(struct epoll_event*));
	
	for (i = 0; i < max_clients; ++i) {
		evhand->events[i] = malloc(sizeof(struct epoll_event));
		evhand->events[i]->events = 0;
	}
	
	if ((evhand->fd = epoll_create(max_clients)) == -1) {
		perror ("ERROR epoll_create");
		exit (1);
	}
	
	/* kqueue */
	/* ... */
	
	return evhand;
}

int events_add_event(event_handler *evhand, int fd) {
	/* add a fd to the event list */
	
	/* epoll */
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = fd;

	if (epoll_ctl(evhand->fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
		perror ("ERROR epoll_ctl (EPOLL_CTL_ADD)");
		return -1;
	}
	
	/* kqueue */
	/* ... */
	
	return 0;
}

int events_del_event(event_handler *evhand, int fd) {
	/* delete a fd from the event list */
	
	/* epoll */
	if (epoll_ctl(evhand->fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
		perror ("ERROR epoll_ctl (EPOLL_CTL_DEL)");
		return -1;
	}
	
	/* kqueue */
	/* ... */
	
	return 0;
}

int events_wait(event_handler *evhand, master_server *master_srv) {
	/* wait for an event (new connection, data recieved etc.) */
	int nfds;

	/* epoll */
	if ((nfds = epoll_wait(evhand->fd, evhand->events, master_srv->config->max_clients, -1)) == -1) {
		perror ("ERROR epoll_pwait");
		return -1;
	}

	/* kqueue */
	/* ... */

	return nfds;
}

int events_get_fd(event_handler *evhand, int num) {
	/* return the fd from the event list */
	struct epoll_event *events = evhand->events;
	
	/* epoll */
	return events[num].data.fd;
	
	/* kqueue */
	/* .. */
}

int events_free(event_handler *evhand, int max_clients) {
	/* free an event handler */
	int i;
	
	/* epoll */
	for (i = 0; i < max_clients; ++i) {
		free (evhand->events[i]);
	}

	/* kqueue */
	/* ... */
	
	free (evhand);
	
	return 0;
}

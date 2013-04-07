#include "events.h"

event_handler *events_create(int max_clients) {
	/* create a new event handler (epoll, kqueue etc.) */
	int i;
	event_handler *evhand = malloc(sizeof(event_handler));

	/* epoll */
#ifdef HAVE_SYS_EPOLL_H
	evhand->events = malloc(max_clients * sizeof(struct epoll_event*));
	
	for (i = 0; i < max_clients; ++i) {
		evhand->events[i] = malloc(sizeof(struct epoll_event));
		evhand->events[i]->events = 0;
	}
	
	if ((evhand->fd = epoll_create(max_clients)) == -1) {
		perror ("ERROR epoll_create");
		exit (1);
	}
#elif defined(HAVE_SYS_EVENT_H)
	/* kqueue */
	evhand->events = malloc(max_clients * sizeof(struct kevent*));
	
	for (i = 0; i < max_clients; ++i) {
		evhand->events[i] = malloc(sizeof(struct kevent));
	}
	
	evhand->nevents = 0;
	
	if ((evhand->fd = kqueue()) == -1) {
		perror ("ERROR kqueue");
		exit (1);
	}
#endif

	return evhand;
}

int events_add_event(event_handler *evhand, int fd) {
	/* add a fd to the event list */
	
	/* epoll */
#ifdef HAVE_SYS_EPOLL_H
	struct epoll_event ev;
	ev.events = EPOLLIN;
	ev.data.fd = fd;

	if (epoll_ctl(evhand->fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
		perror ("ERROR epoll_ctl (EPOLL_CTL_ADD)");
		return -1;
	}
#elif defined(HAVE_SYS_EVENT_H)
	/* kqueue */
	EV_SET (&evhand->changes, fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
	kevent (evhand->fd, &evhand->changes, 1, (void *)0, 0, (struct timespec*)0);
#endif

	return 0;
}

int events_add_event_2(event_handler *evhand, int fd) {
	/* add a fd to the event list */
	
	/* epoll */
#ifdef HAVE_SYS_EPOLL_H
	struct epoll_event ev;
	ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
	ev.data.fd = fd;

	if (epoll_ctl(evhand->fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
		perror ("ERROR epoll_ctl (EPOLL_CTL_ADD)");
		return -1;
	}

#elif defined(HAVE_SYS_EVENT_H)
	/* kqueue */
	EV_SET (&evhand->changes, fd, EVFILT_READ, EV_ADD | EV_ENABLE, 0, 0, 0);
	kevent (evhand->fd, &evhand->changes, 1, (void *)0, 0, (struct timespec*)0);
#endif

	return 0;
}

int events_del_event(event_handler *evhand, int fd) {
	/* delete a fd from the event list */
	
	/* epoll */
#ifdef HAVE_SYS_EPOLL_H
	if (epoll_ctl(evhand->fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
		perror ("ERROR epoll_ctl (EPOLL_CTL_DEL)");
		return -1;
	}

#elif defined(HAVE_SYS_EVENT_H)
	/* kqueue */
	/* ... */
#endif

	return 0;
}

int events_wait(event_handler *evhand, master_server *master_srv) {
	/* wait for an event (new connection, data recieved etc.) */
	int nfds;

	/* epoll */
#ifdef HAVE_SYS_EPOLL_H
	if ((nfds = epoll_wait(evhand->fd, evhand->events, master_srv->config->max_clients, -1)) == -1) {
		perror ("ERROR epoll_pwait");
		return -1;
	}
	
#elif defined(HAVE_SYS_EVENT_H)
	/* kqueue */
	if ((nfds = kevent(evhand->fd, (void *)0, 0, evhand->events, master_srv->config->max_clients, NULL) == -1) {
		perror ("ERROR kevent");
		return -1;
	}
#endif

	return nfds;
}

int events_closed(event_handler *evhand, int num) {
	/* check if this fd has closed its connection */
	
	/* epoll */
#ifdef HAVE_SYS_EPOLL_H
	struct epoll_event *events = evhand->events;
	return events[num].events & EPOLLRDHUP;

#elif defined(HAVE_SYS_EVENT_H)
	/* kqueue */
	struct kevent *events = evhand->events;
	return events[num]->flags & EV_EOF
#endif
}

int events_get_fd(event_handler *evhand, int num) {
	/* return the fd from the event list */
	
	/* epoll */
#ifdef HAVE_SYS_EPOLL_H
	struct epoll_event *events = evhand->events;
	return events[num].data.fd;

#elif defined(HAVE_SYS_EVENT_H)
	/* kqueue */
	struct kevent *events = evhand->events;
	return events[i].ident;
#endif
}

int events_free(event_handler *evhand, int max_clients) {
	/* free an event handler */
	int i;
	
	/* epoll */
#ifdef HAVE_SYS_EPOLL_H
	for (i = 0; i < max_clients; ++i) {
		free (evhand->events[i]);
	}

#elif defined(HAVE_SYS_EVENT_H)
	/* kqueue */
	for (i = 0; i < max_clients; ++i) {
		free (evhand->events[i]);
	}
#endif

	free (evhand);
	
	return 0;
}

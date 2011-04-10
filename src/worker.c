#include "worker.h"

worker *worker_init(master_server *master_srv, int num) {
	/* init a new worker */
	int shmid;
	key_t key = SHARED_MEM_KEY+num;
	worker *w = NULL;

	/* create a shared memory */
	if ((shmid = shmget(key, sizeof(worker), IPC_CREAT | 0666)) < 0) {
		perror ("ERROR shmget");
		return NULL;
	}

	/* attach it */
	if ((w = shmat(shmid, NULL, 0)) == (char*) -1) {
		perror ("ERROR shmat");
		return NULL;
	}
	
	return w;
}

int worker_free(worker *w) {
	/* detach the shared memory */
	if (shmdt(w) == -1) {
		perror ("ERROR shmdt");
		return -1;
	}
	
	return 0;
}

int worker_spawn(int number, master_server *master_srv) {
	/* spawn a new worker process */
	void **child_stack = malloc(master_srv->config->child_stack_size);
	worker *w = master_srv->workers[number];

	w->num = number;
	w->master_srv = master_srv;
	
	/* TODO: use rfork() on FreeBSD & OpenBSD */ 
	if (clone(worker_server, child_stack+sizeof(child_stack) / sizeof(*child_stack), CLONE_FS | CLONE_FILES, w) == -1) { //| CLONE_VM
		perror ("ERROR clone()");
		return -1;
	}
	
	return 0;
}

static int worker_server(worker *info) {
	/* server worker */
	int nfds, fd, i;
	connection **conns; /* connections */
	worker *w = NULL;

	int num = info->num;
	master_server *master_srv = info->master_srv;

	/* attach the shared mem */	
	int shmid;
	key_t key = SHARED_MEM_KEY+num;

	if ((shmid = shmget(key, sizeof(worker), 0666)) < 0) {
		perror ("ERROR shmget");
		exit (1);
	}

	/* attach it */
	if ((w = shmat(shmid, NULL, 0)) == (char*) -1) {
		perror ("ERROR shmat");
		exit (1);
	}
	
	/* process id */
	w->pid = getpid();
	
	/* worker process started */
	printf ("Worker process #%d started.\n", num+1);
	
	/* pre-setup worker connections */
	conns = connection_setup(master_srv);
	
	/* event stuff (epoll, kqueue) */
	epoll_struct *event_handler = malloc(sizeof(epoll_struct));
	event_handler->events = malloc(master_srv->config->max_clients * sizeof(struct epoll_event*));
	
	struct epoll_event *events = event_handler->events;
	struct epoll_event ev;
	
	for (i = 0; i < master_srv->config->max_clients; ++i) {
		event_handler->events[i] = malloc(sizeof(struct epoll_event));
		event_handler->events[i]->events = 0;
	}
	
	if ((event_handler->fd = epoll_create(master_srv->config->max_clients)) == -1) {
		perror ("ERROR epoll_create");
		exit (1);
	}

	/* share the event fd */
	w->event_fd = event_handler->fd;
	
	/* entering main loop... */
	while (master_srv->running) {
		/* check for new data */
		if ((nfds = epoll_wait(event_handler->fd, events, master_srv->config->max_clients, -1)) == -1) {
			perror ("ERROR epoll_pwait");
		}

		for (i = 0; i < nfds; ++i) {
			/* data received */
			fd = events[i].data.fd;
			connection *conn = conns[fd];
			
			if (events[i].events & EPOLLRDHUP) {
				/* the other end closed the connection */
				conn->status = CONN_INACTIVE;
			} 
			else if (conn->status == CONN_INACTIVE) {
				/* this connection is inactive, initiate a new connection */
				connection_start (master_srv, conn);
			}
			
			connection_handle (conn);
			
			/* closing */
			if (conn->status == CONN_INACTIVE) {
				if (epoll_ctl(event_handler->fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
					perror ("ERROR epoll del");
				}

				close (fd);
			}
		}
	}
	
	printf ("Shutting down process #%d...\n", num+1);
	
	/* free event stuff (epoll, kqueue) */
	for (i = 0; i < master_srv->config->max_clients; ++i) {
		free (event_handler->events[i]);
	}
	
	free (event_handler->events);
	free (event_handler);

	/* TODO: free all connections */
	/* ... */
	free (conns);

	/* free this workers memory */
	worker_free (w);

	exit (0);
}


#include "worker.h"

worker *worker_init(master_server *master_srv, int num) {
	/* init a new worker */
	int shmid;
	key_t key = master_srv->pid + num;
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
	
	w->num = num;
	w->master_srv = master_srv;
	
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

	/* TODO: use rfork() on FreeBSD & OpenBSD */ 
	if (clone(worker_server, child_stack+sizeof(child_stack) / sizeof(*child_stack), CLONE_FS | CLONE_FILES, w) == -1) {
		perror ("ERROR clone()");
		return -1;
	}

	return 0;
}

static int worker_server(worker *info) {
	/* server worker */
	int nfds, fd, i;
	worker *w;

	int num = info->num;
	master_server *master_srv = info->master_srv;

	/* attach the shared mem */	
	int shmid;
	key_t key = master_srv->pid + num;

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
	printf (" * Worker process #%d is started.\n", num+1);
	
	/* pre-setup worker connections */
	w->conns = connection_setup(master_srv);
	
	/* create a new event handler for this worker */
	event_handler *ev_handler = events_create(master_srv->config->max_clients);

	/* share the event fd */
	w->ev_handler.fd = ev_handler->fd;
	
	/* starting keep-alive clean-up thread */
	printf (" * Starting keep-alive clean-up thread for worker #%d.\n", num+1);
	pthread_t thread_keep_alive;
	int rc_cleanup = pthread_create(&thread_keep_alive, NULL, keep_alive_cleanup, w);
	
	/* entering main loop... */
	while (master_srv->running) {
		/* check for new data */
		if ((nfds = events_wait(ev_handler, master_srv)) == -1) {
			perror ("ERROR epoll_pwait");
		}

		for (i = 0; i < nfds; ++i) {
			/* data received */
			fd = events_get_fd(ev_handler, i);
			connection *conn = w->conns[fd];
			
			if (events_closed(ev_handler, i)) {
				/* the other end closed the connection */
				conn->status = CONN_INACTIVE;
				printf (" * Other end closed the connection\n");
			} else if (conn->status == CONN_INACTIVE) {
				/* this connection is inactive, initiate a new connection */
				connection_start (master_srv, conn);
			}
			
			connection_handle (w, conn);
			
			/* closing */
			if (conn->status == CONN_INACTIVE) {
				if (events_del_event(ev_handler, fd) == -1) {
					perror ("ERROR events_del_event");
				}
				close (fd);
			}
		}
	}
	
	printf (" * Shutting down worker process #%d...\n", num+1);
	
	/* free event handler */
	events_free (ev_handler, master_srv->config->max_clients);

	/* TODO: free all connections */
	free (w->conns);

	/* free this workers memory */
	worker_free (w);

	exit (0);
}

void *keep_alive_cleanup(worker *w) {
	/* garbage collector for keep-alive connections */
	int i;
	master_server *master_srv = w->master_srv;
	time_t ts;
	connection *conn;
	
	while (1) {
		printf (" * Starting keep-alive clean-up...\n");
		ts = time(NULL);
		
		for (i = 0; i < master_srv->config->max_clients*2; ++i) {
			conn = w->conns[i];
			
			if (conn->status != CONN_INACTIVE) {
				if (ts - conn->last_event >= 5) {
					printf (" * fd:%d Keep-alive connection timed out.\n", conn->fd);
					
					/* reset connection */
					conn->status = CONN_CLOSED;
					connection_reset (master_srv, conn);
					conn->status = CONN_INACTIVE;
					
					/* close connection */
					if (events_del_event(&w->ev_handler, conn->fd) == -1) {
						perror ("ERROR events_del_event");
					}
					close (conn->fd);
				}
			}
		}
		
		/* sleep before next cycle */
		sleep (master_srv->config->keep_alive_timeout);
	}
	
	/* we will probably never get here... */
	pthread_exit (NULL);
}

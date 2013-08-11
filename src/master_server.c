#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "config.h"

#include <sys/mman.h>

#include "base.h"

master_server *master_serv; /* used by signal handler */


static void signal_handler(int sig) {
	if (sig == SIGCHLD) {
		/* child exited, should start it again */
		printf ("WARNING a child process was shutdown!\n");
	} else {
		/* shutdown the whole server */
		master_serv->running = 0;
	}
}

void *heartbeat_monitor(void *p) {
	/* heartbeat monitor */
	int i;
	time_t ts = 0;
	master_server *master_srv = p;
	
	while (1) {
		ts = time(NULL);
	
		for (i = 0; i < master_srv->config->max_workers; ++i) {
			worker *w = master_srv->workers[i];
			if (ts - w->heartbeat > master_srv->config->heartbeat_interval*2) {
				printf ("WARNING worker #%d (pid %d) is dead! Respawning...\n", i, w->pid);
				
				close (w->ev_handler.fd);
				worker_spawn (i, master_srv);
			}
		}
		
		sleep (master_srv->config->heartbeat_interval);
	}
	pthread_exit (NULL);
}

static void daemonize() {
	/* daemonize server */
	if (fork() != 0) {
		exit (0);
	}

	umask (0);

	if (setsid() == -1) {
		perror ("ERROR setsid");
		exit (1);
	}
	
	/* close standard file descriptors */
	//close (STDIN_FILENO);
        close (STDOUT_FILENO);
        close (STDERR_FILENO);
}

int master_server_init(master_server *master_srv) {
	/* init the master server */
	int i;
	
	/* init locks */
	lock_init (&master_srv->lock_log);
	
	/* process id */
	master_srv->pid = getpid();
	
	/* allocate worker pointers */
	master_srv->workers = malloc(master_srv->config->max_workers * sizeof(worker*));
	
	for (i = 0; i < master_srv->config->max_workers; ++i) {
		if ((master_srv->workers[i] = worker_init(master_srv, i)) == NULL) {
			perror ("ERROR creating a new worker");
			return -1;
		}
	}
	
	/* map workers to fds */
	master_srv->worker_map = malloc(sizeof(worker) * master_srv->config->max_clients * 2);
	for (i = 0; i < master_srv->config->max_clients * 2; ++i) {	
		master_srv->worker_map[i] = master_srv->workers[ i % master_srv->config->max_workers ];
	}
	
	/* allocate server pointers (1000) */
	master_srv->servers = malloc(sizeof(server*) * 1000);
	master_srv->server_count = 0;
	master_srv->server_default = 0;
	
	/* allocate server directories (host=>server) */
	master_srv->servers_by_port = malloc(sizeof(cache));
	cache_init (master_srv->servers_by_port);
	
	master_srv->servers_by_fd = malloc(sizeof(server) * master_srv->config->max_clients * 2);
	
	master_srv->servers_by_host = malloc(sizeof(cache));
	cache_init (master_srv->servers_by_host);
	
	return 0;
}

int master_server_free(master_server *master_srv) {
	/* free everything allocated in master_server_init */
	int i;
	
	/* free worker pointers */
	for (i = 0; i < master_srv->config->max_workers; ++i) {
		worker_free (master_srv->workers[i]);
	}
	
	free (master_srv->workers);
	free (master_srv->worker_map);
	
	/* free server pointers */
	free (master_srv->servers);
	
	/* free server directories */
	cache_free (master_srv->servers_by_port);
	free (master_srv->servers_by_fd);
	cache_free (master_srv->servers_by_host);
	
	/* free locks */
	lock_free (&master_srv->lock_log);
	
	/* free the master server */
	free (master_srv);
	
	return 0;
}

int main(int argc, char *argv[]) {
	int nfds, fd, i, c;
	int arg_daemonize = 0;
	
	/* lock all memory in physical RAM */
	if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
		perror ("ERROR mlockall");
	}
	
	/* create a new master server */
	master_server *master_srv = malloc(sizeof(master_server));
	
	/* setup signal handlers */
	signal (SIGHUP, signal_handler);
	signal (SIGTERM, signal_handler);
	signal (SIGINT, signal_handler);
	signal (SIGKILL, signal_handler);
	signal (SIGQUIT, signal_handler);
	signal (SIGCHLD, signal_handler); /* child exited */
	
	/* parse command args (to get to config path) */
	while ((c = getopt(argc, argv, "c:d")) != -1) {
		switch (c) {
			case 'c':
				master_srv->config_file = strdup(optarg);
			break;
			
			case 'd': 
				/* daemonize */
				arg_daemonize = 1;
			break;
			
			case '?':
				if (optopt == 'c') {
					fprintf (stderr, "Option -%c requires an argument.\n", optopt);
				}
				
				exit (1);
			break;
		}
	}

	/* load master server settings from file */
	printf (" * Loading configuration file '%s'...\n", master_srv->config_file);
	master_srv->config = config_init();
	
	if (config_load(master_srv->config_file, master_srv->config) == -1) {
		perror ("ERROR loading config file");
		exit (1);
	}

	/* initiate the master server */
	if (master_server_init(master_srv) == -1) {
		exit (1);
	}
	
	master_srv->running = 1;

	/* daemonize? */
	if (master_srv->config->daemonize || arg_daemonize) {
		printf (" * Daemonizing...\n");
		daemonize();
	}

	/* chroot? */
	if (strlen(master_srv->config->chroot) > 0) {
		if (chdir(master_srv->config->chroot) != 0) {
			perror ("ERROR chdir");
			exit (1);
		}
		if (chroot(master_srv->config->chroot) != 0) {
			perror ("ERROR chroot");
			exit (1);
		}
	} else {
		if (chdir("/") != 0) {
			perror ("ERROR chdir");
			exit (1);
		}
	}
	
	/* create a new event handler for listening */
	event_handler *ev_handler = events_create(master_srv->config->max_clients);
	
	/* loading virtual servers from config */
	printf (" * Loading virtual servers...\n");
	config_load_servers (master_srv->config_file, master_srv);
	printf ("    - %d virtual server(s) where found.\n", master_srv->server_count);
	
	for (i = 0; i < master_srv->server_count; ++i) {
		server *srv = master_srv->servers[i];
	
		/* initiate the new server */
		server_init (srv);
		srv->running = 1;
		srv->master = master_srv;
	
		/* categorize this server by port, hostnames etc. */
		char *port = malloc(7);	
		int port_len = sprintf(port, "%d", srv->config->listen_port);	
		
		/* by port */
		if (cache_exists(master_srv->servers_by_port, port, port_len) != 0) {
			/* this is the first, add it and start listen on that port */
			printf (" * Listening on port %s.\n", port);
			
			if (socket_listen(srv) == -1) {
				perror ("ERROR socket_listen");
				exit (1);
			}
		
			/* add it to the event handler */
			if (events_add_event(ev_handler, srv->server_socket->fd) == -1) {
				perror ("ERROR events_add_event");
				exit (1);
			}

			cache_add (master_srv->servers_by_port, port, srv);
			
			/* also categorize the fd */
			master_srv->servers_by_fd[srv->server_socket->fd] = srv;
		}
		
		free (port);
		
		/* by hostnames */
		if (strlen(srv->config->hostname) > 0) {
			/* hostnames are seperated by space */
			int e, offset = 0;
			
			for (e = 0; e < strlen(srv->config->hostname); ++e) {
				if (srv->config->hostname[e] == ' ' || e == strlen(srv->config->hostname)-1) {
					if (e == strlen(srv->config->hostname)-1) {
						++e;
					}
				
					char *tmp_hostname = malloc(e-offset+1);
					memcpy (tmp_hostname, srv->config->hostname+offset, e-offset);
					tmp_hostname[e-offset] = '\0';
					
					if (srv->config->hostname[e] == ' ') {
						++e;
						offset = e;
					}
					
					/* add it */
					cache_add (master_srv->servers_by_host, tmp_hostname, srv);
			
					/* also by hostname:port */
					char *host_port = malloc(strlen(tmp_hostname)+8);
					sprintf(host_port, "%s:%d", tmp_hostname, srv->config->listen_port);
			
					cache_add (master_srv->servers_by_host, host_port, srv);
			
					free (tmp_hostname);
					free (host_port);
				}
			}
		}
	}
	
	/* starting workers */
	for (i = 0; i < master_srv->config->max_workers; ++i) {	
		worker_spawn (i, master_srv);
		usleep (1);
	}
	usleep (500000);
	
	/* starting heartbeat checking thread */
	printf (" * Starting heartbeat monitor thread.\n");
	pthread_t thread_heartbeat;
	pthread_create(&thread_heartbeat, NULL, heartbeat_monitor, master_srv);
	
	printf (" * The server is up and running!\n");
	
	/* the server is up and running, entering the main loop... */
	while (master_srv->running) {
		/* wait for a new connection */
		if ((nfds = events_wait(ev_handler, master_srv)) == -1) {
			perror ("ERROR epoll_pwait");
		}
		
		for (i = 0; i < nfds; ++i) {
			/* new connection */
			fd = events_get_fd(ev_handler, i);

			/* get the server that is listening on this fd */
			server *srv = master_srv->servers_by_fd[fd];
			printf (" * fd:%d New connection on server '%s'.\n", fd, srv->config->hostname);
			
			/* accept the connection */
			if ((fd = socket_accept(srv->server_socket)) == -1) {
				perror ("ERROR on accept");
			} else {
				/* get the worker for this fd */
				worker *w = master_srv->worker_map[fd];
				
				/* set socket options */
				socket_setup (master_srv, fd);
	
				/* add it to the worker's event handler */
				if (events_add_event_2(&w->ev_handler, fd) == -1) {
					perror ("ERROR events_add_event_2");
				}
			}
		}
	}
	
	/* wait for all child processes to exit */
	for (i = 0; i < master_srv->config->max_workers; ++i) {
		worker *w = master_srv->workers[i];
		waitpid (w->pid, NULL, 0);
	}
	
	/* close server connections */
	//socket_close (srv->server_socket);
	
	/* free the master server */
	master_server_free (master_srv);
	free (master_srv->config_file);
	
	printf (" * The server is down, exiting...\n");
	
	return 0; 
}

#include "connection.h"

connection *connection_init() {
	/* init connection struct */
    	return malloc(sizeof(connection));
}

connection *connection_setup(master_server *master_srv) {
	/* pre-setup connections (for performance) */
	connection **conns = malloc(master_srv->config->max_clients * 2 * sizeof(connection*));
	
	int i;
	for (i = 0; i < master_srv->config->max_clients*2; ++i) {
		conns[i] = connection_init();
		conns[i]->status = CONN_INACTIVE;
		conns[i]->fd = i;
		conns[i]->server = master_srv->servers[master_srv->server_default];
		conns[i]->last_event = 0;
	}
	
	return conns;
}

int connection_start(master_server *master_srv, connection *conn) {
	/* start a new connection */
	conn->status = CONN_STARTED;
	conn->start_ts = NULL; //time(NULL); not used anyway... // TODO: cache the time
	conn->last_event = time(NULL); // TODO: cache the time
	conn->buffer_len = 0;
	conn->read_buffer = NULL;
	conn->request = NULL;
	conn->response = NULL;
	
	return 0;
}

int connection_handle(worker *w, connection *conn) {
	/* handle a connection event */
	server *srv = conn->server;

	/* update ts */
	conn->last_event = time(NULL);	
	
	if (conn->status == CONN_STARTED) {
		conn->status = CONN_READING;
	}

	if (conn->status == CONN_READING) {
		/* read the request */
		if (conn->buffer_len == 0) {
			conn->read_buffer = malloc(srv->master->config->read_buffer_size+1);
		}
		
		if (srv->config->read_buffer_size - conn->buffer_len > 0) {
			/* read from socket */
			int s;
			if ((s = socket_read(conn->fd, conn->read_buffer+conn->buffer_len,
			    srv->master->config->read_buffer_size - conn->buffer_len)) != -1) {
				conn->buffer_len += s;
			} else {
				perror ("ERROR reading from socket");
				conn->status = CONN_ERROR;
			}
		} else {
			/* too big request */
			safe_warn (srv, "request size is too big.");
			conn->status = CONN_ERROR;
		}
		
		/* if all data is received, parse the request */
		if (conn->buffer_len > 4) {
			if (conn->read_buffer[conn->buffer_len-4] == '\r' && conn->read_buffer[conn->buffer_len-3] == '\n' &&
			    conn->read_buffer[conn->buffer_len-2] == '\r' && conn->read_buffer[conn->buffer_len-1] == '\n') {
				/* parse and build a request object */
				request_parse (srv, conn);
				
				/* if host is known, switch server */
				if (conn->request->host_len > 0) {
					if ((conn->server = cache_get_exists(srv->master->servers_by_host, conn->request->host, conn->request->host_len)) == NULL) {
						/* didn't found the requested host */
						conn->server = srv;
					}
					#if 0
					else {
						server *tmp = conn->server;
						if (conn->port != tmp->config->listen_port) {
							/* the requested host is on another port, drop connection */
							conn->status = CONN_ERROR;
						}
					}
					#endif
					srv = conn->server;
				}
				
				if (conn->status != CONN_ERROR) {
					conn->status = CONN_WRITING;
				}
			}
		}
	}

	if (conn->status == CONN_WRITING) {	
		/* build the response data */
		response_build (srv, conn);

		/* send to browser */
		if (conn->response->cached) {
			/* using sendfile on a cached file */
			off_t offset = 0;
			if (sendfile(conn->fd, conn->response->file->fd, &offset, conn->response->http_packet_len) == -1) {
				perror ("ERROR sendfile");
				conn->status = CONN_ERROR;
			}
		} else {
			/* TODO: use send + sendfile */
			if (conn->response->http_packet_len <= srv->master->config->write_buffer_size) {
				/* small buffer, send it with one call */
				if (send(conn->fd, conn->response->http_packet, conn->response->http_packet_len, 0) == -1) {
					perror ("ERROR writing to socket");
					conn->status = CONN_ERROR;
				}
			} else {
				/* large buffer, chunk it */
				unsigned long data_sent = 0;
				int flag, size, sent;
				
				/* TODO: TCP_CORK or MSG_MORE? */
				while (data_sent < conn->response->http_packet_len) {
					if ((size = conn->response->http_packet_len - data_sent) >= srv->master->config->write_buffer_size) {
						size = srv->master->config->write_buffer_size;
						flag = MSG_MORE;
					} else {
						flag = 0;
					}
				
					if ((sent = send(conn->fd, conn->response->http_packet+data_sent, size, flag)) == -1) {
						perror ("ERROR writing to socket");
						conn->status = CONN_ERROR;
						break;
					}
				
					data_sent += size;
				}
			}
		}

		/* keep-alive? */
		if (conn->request->keep_alive && srv->config->keep_alive) {
			conn->status = CONN_INACTIVE;
		} else {
			/* closing connection */
			conn->status = CONN_CLOSED;
		}
	}
	
	if (conn->status == CONN_CLOSED) {
		/* connection is closed */
		conn->status = CONN_INACTIVE;	
	}

	if (conn->status == CONN_ERROR) {
		/* connection error */
		conn->status = CONN_INACTIVE;
	}
	
	if (conn->status == CONN_INACTIVE) {
		/* reset the connection */
		connection_reset (srv->master, conn);
		
		/* should we turn off the cache yet? */
		if (srv->config->cache_files == CACHE_YES) {
			/* check if the upper limit is reached */
			if (srv->cache_used_mem >= srv->config->cache_turn_off_limit) {
				srv->config->cache_files = CACHE_NONE;
				safe_warn (srv, "cache is turned off: upper limit reached.");
			}
		} else if (srv->config->cache_files == CACHE_FD) {
			/* check if the max number of open fds is reached */
			if (srv->cache_open_fds >= srv->config->cache_max_fds) {
				srv->config->cache_files = CACHE_NONE;
				safe_warn (srv, "cache is turned off: max number of open fds has been reached.");
			}
		}
	}

	return 0;
}

int connection_reset(master_server *master_srv, connection *conn) {
	/* reset a connection */
	conn->start_ts = 0;
	conn->buffer_len = 0;
	conn->server = master_srv->servers[master_srv->server_default];
	
	if (conn->status != CONN_CLOSED) {
		conn->status = CONN_INACTIVE;
		
		/* keep-alive? */
		if (((server*)conn->server)->config->keep_alive) {
			if (conn->request != NULL && conn->request->keep_alive) {
				conn->status = CONN_READING;
			}
		}
	}

	if (conn->read_buffer != NULL) {
		free (conn->read_buffer);
		conn->read_buffer = NULL;
	}

	if (conn->request != NULL) {
		request_free (conn->request);
		conn->request = NULL;
	}

	if (conn->response != NULL) {
		response_free (conn->response);
		conn->response = NULL;
	}

	return 0;
}

int connection_free(master_server *master_srv, connection *conn) {
	/* when the server shuts down, all connections need to be freed */
	conn->start_ts = 0;
	conn->last_event = 0;
	conn->buffer_len = 0;
	conn->status = CONN_INACTIVE;

	if (conn->read_buffer != NULL) {
		free (conn->read_buffer);
		conn->read_buffer = NULL;
	}

	if (conn->request != NULL) {
		request_free (conn->request);
		conn->request = NULL;
	}

	if (conn->response != NULL) {
		response_free (conn->response);
		conn->response = NULL;
	}

	free (conn);

	return 0;
}
